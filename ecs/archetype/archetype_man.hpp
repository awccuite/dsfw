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
class ArchetypeManager {
public:
    template<typename Component>
    static ComponentID getComponentID(){
        static ComponentID id = _idCounter++; // Static inside a template function is instantiated once per type
        return id;
    }

    // Should typically be handled in the init phase of a project, so not too worried about the RTTI cost.
    template<typename C>
    bool registerComponent(){
        static bool registered = false; // Static per type registration check
        if(registered){
            std::cout << "Failed to register component, already member" << std::endl;
            return false;
        }

        ComponentID id = getComponentID<C>();
        _componentNames[id] = typeid(C).name();
        registered = true;
        return true;
    }

private:

    static inline std::unordered_map<ComponentID, std::string> _componentNames;
    static inline std::atomic<ComponentID> _idCounter;
};

}; // namespace gxe