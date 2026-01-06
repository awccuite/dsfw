#pragma once

#include <iostream>

#include "types.hpp"
#include "id_manager.hpp"

#include "archetype/archetype_man.hpp"
#include "systems/system_manager.hpp"

#include <any>
#include <cstdint>

namespace gxe {

class world {
public:
     struct entity_record {
        archetype_id global_id = NULL_ARCHETYPE_ID; // Global entity ID
        archetype_id arch_index = NULL_ARCHETYPE_ID; // Index in archetype.
        archetype::signature signature; // Signature of the archetype the entity belongs to.

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
        std::cout << "Registered component: " << typeid(C).name() << " with id: " << id << std::endl;

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
        entity_builder& add_component(Args&&... args) { // Universal reference args for perfect forwarding.
            component_id id = archetype_manager::get_component_id<C>();
            _componentData[id] = std::make_any<C>(std::forward<Args>(args)...); // Create our component data

            return *this;
        }

        // Should build/return an entity record.
        entity_record build() {
            archetype::signature sig = build_signature();
            archetype& arch = _world._archetypes.get_or_create_archetype(sig);

            // Get the arch_index before inserting (this entity will be at the end)
            auto& entity_list = _world._archetypeEntities[sig];
            uint32_t arch_index = static_cast<uint32_t>(entity_list.size());

            // Add each component to our archetype
            for(auto& [c_id, component] : _componentData){
                arch.insert_component(c_id, std::move(component));                
            }

            // Track this entity in the world's reverse lookup
            entity_list.push_back(_id);

            // Create and store the entity record
            entity_record record{_id, arch_index};
            record.signature = sig;
            _world._entityRecords.emplace(_id, record);

            return record;
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

    void delete_entity(entity_id id) {
        auto it = _entityRecords.find(id);
        if (it == _entityRecords.end()) return;
        
        entity_record& record = it->second;
        auto& arch = _archetypes.get_or_create_archetype(record.signature);
        auto& entity_list = _archetypeEntities[record.signature];
        
        size_t deleted_arch_index = record.arch_index;
        size_t last_arch_index = entity_list.size() - 1;
        
        // Perform the swap-and-pop on component data
        auto swapped_old_index = arch.delete_entity_components(deleted_arch_index);
        
        // Update entity tracking: swap-and-pop the entity_list too
        if (swapped_old_index.has_value()) {
            // An entity was swapped - update its record
            entity_id swapped_entity = entity_list[last_arch_index];
            _entityRecords.at(swapped_entity).arch_index = deleted_arch_index;
            
            // Swap in entity_list
            entity_list[deleted_arch_index] = swapped_entity;
        }
        entity_list.pop_back();
        
        // Free the ID and remove the record
        _idManager.destroy_entity(id);
        _entityRecords.erase(id);
    }

    // Need to iterate over each 
    template<typename ...Components>
    void for_each_with_components(){

    }

    // System management methods. 
    void step([[maybe_unused]] float dt) {}; // Method to step our systems by some delta t. Takes seconds as a float

private:
    id_manager _idManager;
    archetype_manager _archetypes;

    std::vector<std::unique_ptr<system_base>> _systems;
    
    // Map from entity_id -> entity_record
    std::unordered_map<entity_id, entity_record> _entityRecords;
    
    // Map from (signature, arch_index) -> entity_id for reverse lookup
    // Needed to find which entity was swapped during deletion
    std::unordered_map<archetype::signature, std::vector<entity_id>, archetype::signature::hash> _archetypeEntities;
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

    std::cout << "\n=== ECS Tests Complete ===" << std::endl;
}

} // namespace debug

#endif // BUILD_DEBUG

}; // namespace gxe