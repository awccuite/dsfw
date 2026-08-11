#pragma once

#include <iostream>

#include "types.hpp"
#include "id_manager.hpp"

#include "archetype/archetype_man.hpp"
#include "systems/system.hpp"

#include <any>
#include <cstdint>
#include <concepts>
#include <type_traits>
#include <utility>
#include <vector>
#include <memory>
#include <chrono>

namespace gxe {

class world {
public:
     struct entity_record {
        archetype_id global_id = NULL_ARCHETYPE_ID; // Global entity ID
        archetype_id arch_index = NULL_ARCHETYPE_ID; // Index in archetype.
        // Index into the archetype manager, replacing a per-entity signature copy
        // (which cost an unordered_set allocation on every single spawn).
        archetype_id arch_id = NULL_ARCHETYPE_ID;

        entity_record(const size_t global, const archetype_id arch_idx) : global_id(global), arch_index(arch_idx) {};

        [[nodiscard]] bool is_active() const { return arch_index != NULL_ARCHETYPE_ID && global_id != NULL_ARCHETYPE_ID; };

        friend std::ostream& operator<< (std::ostream& os, const entity_record& er){
            os << "entity_record(global_id=" << er.global_id << ", arch_index=" << er.arch_index << ")";
            return os;
        }
    };

    // Register a factory method for building component_array<C>
    // Register a component type C, and return the component_id.
    template<typename C>
    component_id register_component(){ // Wrapper around the Archetype manager component call
        auto id = _archetypes.register_component<C>();
        return id;
    }

    template<typename C>
    component_id get_component_id(){
        return archetype_manager::get_component_id<C>();
    }

    // ENTITY BUILDER
    // TODO: Consider optimizations possible for our builder pattern, as spawning many entites may be a bottlneck.
    class entity_builder {
    public:
        entity_builder(world& w, entity_id id) : _world(w), _id(id) {}

        // Add a component to our entity builder.
        template<typename C, typename ...Args> // Add component of type C with arguments Args.
            requires std::constructible_from<C, Args...>
        entity_builder& add_component(Args&&... args) {
            component_id id = archetype_manager::get_component_id<C>();
            _componentData[id] = std::make_any<C>(std::forward<Args>(args)...);

            return *this;
        }

        // Should build/return an entity record.
        entity_record build() {
            const archetype::signature sig = build_signature();
            const size_t a_idx = _world._archetypes.get_or_create_archetype_index(sig);
            archetype& arch = _world._archetypes.archetype_at(a_idx);

            // Add each component to our archetype
            for(auto& [c_id, component] : _componentData){
                arch.insert_component(c_id, std::move(component));
            }

            return _world.finalize_spawn(_id, a_idx);
        }

    private:
        inline archetype::signature build_signature(){
            archetype::signature sig{};

            for(const auto& [comp_id, _] : _componentData){
                sig.insert(comp_id);
            }

            return sig;
        }

        [[maybe_unused]] world& _world;
        entity_id _id; // Global, per world ID. Not position in archetype.
        std::unordered_map<component_id, std::any> _componentData;

    }; // END ENTITY BUILDER

    // Return an entity_builder with an id managed by the _idManager
    entity_builder create_entity() {
        return entity_builder{*this, _idManager.create_entity()};
    };

    // Spawns an entity in one shot: components go straight into their typed columns, with
    // no per-entity std::any map and no signature copy. This is the hot path.
    template <typename ...Components>
    entity_id spawn_entity(Components&&... components) {
        const archetype::signature& sig =
            archetype_manager::signature_for<std::remove_cvref_t<Components>...>();

        const size_t a_idx = _archetypes.get_or_create_archetype_index(sig);
        archetype& arch = _archetypes.archetype_at(a_idx);

        const entity_id id = _idManager.create_entity();
        _archetypes.append_row(arch, std::forward<Components>(components)...);

        return finalize_spawn(id, a_idx).global_id;
    }

    void delete_entity(entity_id id) {
        auto it = _entityRecords.find(id);
        if (it == _entityRecords.end()) return;

        entity_record& record = it->second;
        auto& arch = _archetypes.archetype_at(record.arch_id);
        auto& entity_list = _archetypeEntities[record.arch_id];

        size_t deleted_arch_index = record.arch_index;
        size_t last_arch_index = entity_list.size() - 1;

        // Perform the swap-and-pop on component data
        auto swapped_old_index = arch.delete_entity_components(deleted_arch_index);

        // Update entity tracking: swap-and-pop the entity_list too
        if (swapped_old_index.has_value()) {
            // An entity was swapped - update its record
            entity_id swapped_entity = entity_list[last_arch_index];
            _entityRecords.at(swapped_entity).arch_index = static_cast<archetype_id>(deleted_arch_index);

            // Swap in entity_list
            entity_list[deleted_arch_index] = swapped_entity;
        }
        entity_list.pop_back();

        // Free the ID and remove the record
        _idManager.destroy_entity(id);
        _entityRecords.erase(id);
    }

    // Invokes f(Components&...) for every entity that has all of Components.
    template<typename ...Components, typename F>
    void for_each_with_components(F&& f){
        _archetypes.for_each_with_components<Components...>(std::forward<F>(f));
    }

    // TODO: Replace the flat registration-ordered list with a DAG of system dependencies.
    // A system that ticks would mark its dependents dirty, and a dependent would only run
    // when dirty rather than on its own fixed schedule. Today every system advances purely
    // on its own rate, so a consumer runs whether or not its inputs actually changed -
    // e.g. the draw system redraws several times per frame between physics ticks.
    template<typename S, typename ...Args>
        requires std::derived_from<S, systems::system_base>
    S& register_system(Args&&... args) {
        auto sys = std::make_unique<S>(std::forward<Args>(args)...);
        S& ref = *sys;
        ref.register_components(*this);
        _systems.push_back(std::move(sys));

        return ref;
    }

    // Wall-clock driven: runs each system's pending ticks in registration order.
    void step(std::chrono::nanoseconds elapsed) {
        for (auto& sys : _systems) sys->advance(*this, elapsed);
    }

    // One tick of every system, no clock involved.
    void tick() {
        for (auto& sys : _systems) sys->tick(*this);
    }

private:
    // Shared tail of both spawn paths (spawn_entity and entity_builder::build), so the two
    // cannot drift apart in how they record an entity.
    entity_record finalize_spawn(const entity_id id, const size_t a_idx) {
        if (a_idx >= _archetypeEntities.size()) {
            _archetypeEntities.resize(a_idx + 1);
        }
        auto& entity_list = _archetypeEntities[a_idx];

        const uint32_t arch_index = static_cast<uint32_t>(entity_list.size());

        entity_list.push_back(id);

        entity_record record{id, arch_index};
        record.arch_id = static_cast<archetype_id>(a_idx);
        _entityRecords.insert_or_assign(id, record);

        return record;
    }

    id_manager _idManager;
    archetype_manager _archetypes;

    std::vector<std::unique_ptr<systems::system_base>> _systems;
    std::unordered_map<entity_id, entity_record> _entityRecords;

    // archetype index -> entity_ids in row order. Needed to find which entity was swapped
    // during deletion. Indexed by archetype index rather than keyed by signature, which
    // removes a signature hash+compare from every spawn and every delete.
    std::vector<std::vector<entity_id>> _archetypeEntities;
};

#ifdef BUILD_DEBUG

namespace debug {

struct position {
    float x{0.0f};
    float y{0.0f};
};

struct velocity {
    float vx{0.0f};
    float vy{0.0f};
};

// Only used to force a brand-new archetype after queries have already been cached.
struct health {
    int hp{0};
};

inline void print_record(const std::string& label, const world::entity_record& record) {
    std::cout << label << " -> ID: " << record.global_id 
              << ", arch_index: " << record.arch_index << std::endl;
}

inline void run_ecs_tests() {
    std::cout << "\n=== ECS Entity Tests ===" << std::endl;
    
    world world;
    world.register_component<position>();
    world.register_component<velocity>();

    std::cout << "\n--- Creating initial entities ---" << std::endl;
    
    // Create entities with different component combinations
    auto e1 = world.create_entity()
        .add_component<position>(10.0f, 20.0f)
        .build();
    print_record("Entity 1 (position only)", e1);

    auto e2 = world.create_entity()
        .add_component<velocity>(1.0f, 2.0f)
        .build();
    print_record("Entity 2 (velocity only)", e2);

    auto e3 = world.create_entity()
        .add_component<position>(30.0f, 40.0f)
        .add_component<velocity>(3.0f, 4.0f)
        .build();
    print_record("Entity 3 (position + velocity)", e3);

    auto e4 = world.create_entity()
        .add_component<position>(50.0f, 60.0f)
        .add_component<velocity>(5.0f, 6.0f)
        .build();
    print_record("Entity 4 (position + velocity)", e4);

    auto e5 = world.create_entity()
        .add_component<position>(70.0f, 80.0f)
        .add_component<velocity>(7.0f, 8.0f)
        .build();
    print_record("Entity 5 (position + velocity)", e5);

    std::cout << "\n--- Deleting entity 3 (middle of pos+vel archetype) ---" << std::endl;
    std::cout << "Before delete: e3 arch_index=" << e3.arch_index 
              << ", e5 should swap into e3's position" << std::endl;
    world.delete_entity(e3.global_id);
    std::cout << "Entity 3 deleted. ID " << e3.global_id << " should be recycled next." << std::endl;

    std::cout << "\n--- Deleting entity 1 (only entity in position-only archetype) ---" << std::endl;
    world.delete_entity(e1.global_id);
    std::cout << "Entity 1 deleted. ID " << e1.global_id << " should be recycled next." << std::endl;

    std::cout << "\n--- Creating new entities (should recycle IDs) ---" << std::endl;
    
    auto e6 = world.create_entity()
        .add_component<position>(100.0f, 110.0f)
        .build();
    print_record("Entity 6 (position only)", e6);
    std::cout << "  -> Expected recycled ID: " << e1.global_id << std::endl;

    auto e7 = world.create_entity()
        .add_component<position>(120.0f, 130.0f)
        .add_component<velocity>(12.0f, 13.0f)
        .build();
    print_record("Entity 7 (position + velocity)", e7);
    std::cout << "  -> Expected recycled ID: " << e3.global_id << std::endl;

    auto e8 = world.create_entity()
        .add_component<velocity>(14.0f, 15.0f)
        .build();
    print_record("Entity 8 (velocity only)", e8);
    std::cout << "  -> This should be a fresh ID (no velocity-only deleted)" << std::endl;

    std::cout << "\n--- Stress test: create and delete multiple ---" << std::endl;
    
    // Create 5 more entities
    std::vector<world::entity_record> batch;
    for (int i = 0; i < 5; i++) {
        auto e = world.create_entity()
            .add_component<position>(static_cast<float>(i * 10), static_cast<float>(i * 10))
            .add_component<velocity>(static_cast<float>(i), static_cast<float>(i))
            .build();
        batch.push_back(e);
        print_record("Batch entity " + std::to_string(i), e);
    }

    std::cout << "\n--- Deleting batch entities in reverse order ---" << std::endl;
    for (int i = 4; i >= 0; i--) {
        std::cout << "Deleting batch entity " << i << " (ID: " << batch[i].global_id << ")" << std::endl;
        world.delete_entity(batch[i].global_id);
    }

    std::cout << "\n--- Recreating after batch delete (IDs should recycle) ---" << std::endl;
    for (int i = 0; i < 3; i++) {
        auto e = world.create_entity()
            .add_component<position>(static_cast<float>(i * 100), static_cast<float>(i * 100))
            .build();
        print_record("Recreated entity " + std::to_string(i), e);
    }

    // Query coverage. Deterministic totals - a mis-resolved column, a stale query cache
    // or a desynced archetype row count all show up as a wrong count or sum here.
    std::cout << "\n--- Queries ---" << std::endl;

    size_t pos_count = 0;
    float pos_sum = 0.0f;
    world.for_each_with_components<position>([&](position& p){
        pos_count++;
        pos_sum += p.x + p.y;
    });
    std::cout << "position: count=" << pos_count << ", sum=" << pos_sum << std::endl;

    size_t pv_count = 0;
    float pv_sum = 0.0f;
    world.for_each_with_components<position, velocity>([&](position& p, velocity& v){
        pv_count++;
        pv_sum += p.x + p.y + v.vx + v.vy;
    });
    std::cout << "position+velocity: count=" << pv_count << ", sum=" << pv_sum << std::endl;

    size_t vel_count = 0;
    world.for_each_with_components<velocity>([&](velocity&){ vel_count++; });
    std::cout << "velocity: count=" << vel_count << std::endl;

    // Mutation through the query must be visible to a subsequent query.
    world.for_each_with_components<position>([](position& p){ p.x += 1.0f; });
    float pos_sum2 = 0.0f;
    world.for_each_with_components<position>([&](position& p){ pos_sum2 += p.x + p.y; });
    std::cout << "position after +1 to each x: sum=" << pos_sum2
              << " (expected " << (pos_sum + static_cast<float>(pos_count)) << ")" << std::endl;

    // Query cache lazy-extension: create an archetype that did not exist when the <position>
    // query was first cached, then re-run it. A cache that only populates on first use, or
    // that fails to rescan the new tail of _archetypes, reports the stale count here.
    std::cout << "\n--- Query cache extension (new archetype after first query) ---" << std::endl;
    world.register_component<health>();
    world.create_entity()
        .add_component<position>(1000.0f, 2000.0f)
        .add_component<health>(100)
        .build();

    size_t pos_count3 = 0;
    world.for_each_with_components<position>([&](position&){ pos_count3++; });
    std::cout << "position: count=" << pos_count3
              << " (expected " << (pos_count + 1) << ")" << std::endl;

    size_t hp_count = 0;
    world.for_each_with_components<health>([&](health&){ hp_count++; });
    std::cout << "health: count=" << hp_count << " (expected 1)" << std::endl;

    std::cout << "\n=== ECS Tests Complete ===" << std::endl;
}

} // namespace debug

#endif // BUILD_DEBUG

}; // namespace gxe