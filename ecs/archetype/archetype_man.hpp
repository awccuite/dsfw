#pragma once

#include "archetype.hpp"

#include <memory>
#include <atomic>
#include <string>

namespace gxe {

class archetype_manager {
public:
    template<typename Component>
    static component_id getComponentID(){
        static component_id id = _idCounter++; // Static inside a template function is instantiated once per type
        return id;
    }

    template<typename C>
    uint32_t registerComponent(){
        component_id id = getComponentID<C>();
        _componentNames[id] = typeid(C).name();

        return id;
    }

    // Get or create an archetype based on the requested signature
    archetype& get_or_create_archetype(archetype::signature& signature){
        auto it = _archetypeMap.find(signature);
        if(it != _archetypeMap.end()){
            return *(_archetypes[it->second]._data);
        }

        return create_archetype(signature);
    }

private:
    archetype& create_archetype(archetype::signature& signature){
        archetype_wrapper wrapper;
        wrapper._signature = signature;
        wrapper._data = std::make_unique<archetype>();

        // Create a component array in the archetype for each signature bit.
        

        return *(wrapper._data);
    }

    struct archetype_wrapper {
        archetype::signature _signature;
        std::unique_ptr<archetype> _data;
    };

    // We intentially store duplicates of the signatures, as the map provides O(1) specific archetype lookup, and the vector provides efficient iteration over signatures.
    std::vector<archetype_wrapper> _archetypes; // Archetype container. Useful for queries like "forEachWith<Components...>()". We can always cache queries as well
    std::unordered_map<archetype::signature, size_t, archetype::signature::hash> _archetypeMap; // Map each archetype signature to its index in _archetypes.

    static inline std::unordered_map<component_id, std::string> _componentNames;
    static inline std::atomic<component_id> _idCounter;
};

}; // namespace gxe