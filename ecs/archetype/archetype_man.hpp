#pragma once

#include "archetype.hpp"

#include <memory>
#include <atomic>
#include <string>

namespace gxe {

// Maintains the relationship between archetypes, as well as caches common query
// results and other archtype specific API functionality.

// API
// void registerComponent<Component>(); Is templated so we can have our signature bitsets at compile time.
// Want to lazily initialize component combinations into a literal archetype in order to avoid creating
// n! archetypes from components.

// Maintain a counter for component to id mapping.
class archetype_manager {
public:
    template<typename Component>
    static component_id getComponentID(){
        static component_id id = _idCounter++; // Static inside a template function is instantiated once per type
        return id;
    }

    // Should typically be handled in the init phase of a project, so not too worried about the RTTI cost.
    template<typename C>
    uint32_t registerComponent(){
        component_id id = getComponentID<C>();
        _componentNames[id] = typeid(C).name();

        return id;
    }

private:

    static inline std::unordered_map<component_id, std::string> _componentNames;
    static inline std::atomic<component_id> _idCounter;
};

}; // namespace gxe