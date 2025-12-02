#pragma once

#include "system.hpp"
#include "id_manager.hpp"
#include "archetype/archetype_man.hpp"

#include <any>

namespace gxe {

class world {
public:
    template<typename C>
    component_id registerComponent(){ // Wrapper around the Archetype manager component call
        return _archetypes.registerComponent<C>();
    }

    // Builder as a nested class with access to world
    class entity_builder {
    public:
        entity_builder(world& w, entity_id id) : _world(w), _id(id) {}

        // Add a component to our entity builder.
        template<typename C, typename ...Args>
        entity_builder& add_component(Args&&... args) {
            component_id id = archetype_manager::getComponentID<C>();
            _componentData[id] = std::make_any<C>(std::forward<Args>(args)...); // Create our component data

            return *this;
        }

        entity_id build() {
            // Build the signature from the component ID's.
            // Check if it exists, or we need a new archetype.
            // Add the relevant components at the correct location in the archetype.


            return _id;
        }

    private:
        world& _world;
        entity_id _id;
        std::unordered_map<component_id, std::any> _componentData;
        // You may also need storage for component data
    };

    // Creturn an entity_builder with an id managed by the _idManager
    entity_builder create_entity() {
        return entity_builder(*this, _idManager.create_entity());
    };

private:
    struct entity_record {
        archetype_id localId = NULL_ARCHETYPE_ID; // Default invalid values
        archetype_id archetypeIndex = NULL_ARCHETYPE_ID;

        entity_record(size_t local, archetype_id archId) : localId(local), archetypeIndex(archId) {};

        bool is_active() const { return archetypeIndex != NULL_ARCHETYPE_ID && localId != NULL_ARCHETYPE_ID; };
    };

    id_manager _idManager;
    archetype_manager _archetypes;

    std::vector<std::unique_ptr<system_base>> _systems;

    bool _initialized = false;
};

}; // namespace gxe