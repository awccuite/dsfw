#pragma once

#include "archetype.hpp"

#include <memory>
#include <atomic>
#include <string>
#include <functional>

namespace gxe {

class archetype_manager {
using component_factory = std::function<std::unique_ptr<archetype::component_array_base>()>;

public:
    template<typename Component>
    static component_id get_component_id(){
        static component_id id = _idCounter++; // Static inside a template function is instantiated once per type
        return id;
    }
 
    template<typename C>
    uint32_t register_component(){
        component_id id = get_component_id<C>();
        _componentNames[id] = typeid(C).name();
        
        // Static per C method for creating a component array for said method
        _componentFactories[id] = []() -> std::unique_ptr<archetype::component_array_base> {
            return std::make_unique<archetype::component_array<C>>();
        };

        return id;
    }

    // Get or create an archetype based on the requested signature
    // This method guarantees that the specified
    // archetype (or signature return archetype) has the correct component storage.
    archetype& get_or_create_archetype(archetype::signature& signature){
        auto it = _archetypeMap.find(signature);
        if(it != _archetypeMap.end()){
            return *(_archetypes[it->second]._data);
        }

        return create_archetype(signature); // Create the archetype and return.
    }

private:
    archetype& create_archetype(archetype::signature& signature){
        archetype_wrapper wrapper;
        wrapper._signature = signature;
        wrapper._data = std::make_unique<archetype>();

        for(const component_id& id : signature.components()){
            auto it = _componentFactories.find(id);
            if(it == _componentFactories.end()){
                throw std::runtime_error("Component not registered: " + std::to_string(id));
            }
            wrapper._data->insert_component_array(id, it->second());
        }
        
        size_t index = _archetypes.size();
        _archetypes.push_back(std::move(wrapper));
        _archetypeMap[signature] = index;
        
        return *(_archetypes[index]._data);
    }

    struct archetype_wrapper {
        archetype::signature _signature;
        std::unique_ptr<archetype> _data;
    };

    // We intentially store duplicates of the signatures, as the map provides O(1) specific archetype lookup, and the vector provides efficient iteration over signatures.
    std::vector<archetype_wrapper> _archetypes; // Archetype container. Useful for queries like "forEachWith<Components...>()". We can always cache queries as well
    std::unordered_map<archetype::signature, size_t, archetype::signature::hash> _archetypeMap; // Map each archetype signature to its index in _archetypes.
    std::unordered_map<component_id, component_factory> _componentFactories;

    static inline std::unordered_map<component_id, std::string> _componentNames;
    static inline std::atomic<component_id> _idCounter; // Have id's be consistent per class across all potential instances
};

}; // namespace gxe