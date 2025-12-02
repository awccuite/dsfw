#pragma once

#include <unordered_map>
#include <vector>

#include "../types.hpp"

namespace gxe {

// An archetype is a collection of components, that are related in that each of the
// entities that own one component in the archetype own one of every
// component in the archetype.

// Each archetype maintains a vector for each of its underlying components.

// Each archetype has a signature. Entities themselves to not maintain a signature,
// any time an entity needs to use a signature, it should be provided by the archetype.

class archetype {
public:
    archetype() = default;
    ~archetype() = default;

    // Wrapper around a dynamic bitset (currentl vector<bool>)
    class signature {
        // Check if the archetype contains the component ID.
        bool contains(const component_id id) const { 

        }

        std::vector<bool> _data;
    };

    signature& getSignature() {
        return _signature;
    }

private:
    // Map from componentID to the components index in our collection
    // Type erased container around a component.
    struct component_wrapper {
        
    };

    std::unordered_map<component_id, size_t> _componentMap; // Map from componentid to their index in this specific archetype collection
    signature _signature;
};

}; // namespace gxe