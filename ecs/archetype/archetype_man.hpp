#pragma once

#include "archetype.hpp"

#include <memory>
#include <atomic>
#include <string>

namespace gxe {

// Maintains the relationship between archetypes, as well as caches common query
// results and other archtype specific API functionality.

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
    struct archetype_wrapper {
        archetype::signature _signature;
        std::unique_ptr<archetype> _data;
    };

    // We intentially store duplicates of the signatures, as the map provides O(1) specific archetype lookup, and the vector provides efficient iteration over signatures.
    std::vector<archetype_wrapper> _archetypes; // Archetype container. Useful for queries like "forEachWith<Components...>()". We can always cache queries as well
    std::unordered_map<archetype::signature, size_t> _archetypeIndexMap; // Map each archetype signature to its index in _archetypes.

    static inline std::unordered_map<component_id, std::string> _componentNames;
    static inline std::atomic<component_id> _idCounter;
};

}; // namespace gxe