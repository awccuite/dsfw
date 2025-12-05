#pragma once

#include "types.hpp"
#include "id_manager.hpp"

#include "archetype/archetype_man.hpp"
#include "systems/system_manager.hpp"

#include <any>

namespace gxe {

class world {
private:
    id_manager _idManager;
    archetype_manager _archetypes;

    std::vector<std::unique_ptr<system_base>> _systems;

    [[maybe_unused]] bool _initialized = false;

public:
    struct entity_record {
        archetype_id global_id = NULL_ARCHETYPE_ID; // Global entity ID
        archetype_id arch_index = NULL_ARCHETYPE_ID; // Index in archetype.

        entity_record(size_t global, archetype_id arch_idx) : global_id(global), arch_index(arch_idx) {};

        bool is_active() const { return arch_index != NULL_ARCHETYPE_ID && global_id != NULL_ARCHETYPE_ID; };
    };

    // Register a factory method for building component_array<C>
    template<typename C>
    component_id register_component(){ // Wrapper around the Archetype manager component call
        return _archetypes.register_component<C>();
    }

    template<typename C>
    component_id get_component_id(){
        return archetype_manager::get_component_id<C>();
    }

    // ENTITY BUILDER
    class entity_builder {
    public:
        entity_builder(world& w, entity_id id) : _world(w), _id(id) {}

        // Add a component to our entity builder.
        template<typename C, typename ...Args>
        entity_builder& add_component(Args&&... args) {
            component_id id = archetype_manager::get_component_id<C>();
            _componentData[id] = std::make_any<C>(std::forward<Args>(args)...); // Create our component data

            return *this;
        }

        // Should build/return an entity record.
        entity_record build() {
            archetype::signature sig = build_signature();
            archetype& archetype = _world._archetypes.get_or_create_archetype(sig);

            // Need to add each component to our archetype. 
            for(auto [c_id, component] : _componentData){
                archetype.insert_component(c_id, std::move(component));                
            }

            return {_id, 0}; // Global per world ID
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

    // Creturn an entity_builder with an id managed by the _idManager
    entity_builder create_entity() {
        return entity_builder(*this, _idManager.create_entity());
    };
};

}; // namespace gxe