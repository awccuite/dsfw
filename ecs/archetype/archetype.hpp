#pragma once

#include <unordered_map>
#include <vector>

#include "../types.hpp"

namespace gxe {

// An archetype is a collection of components, that are related in that each of the
// entities that own one component in the archetype own one of every
// component in the archetype.

// Each archetype maintains a vector for each of its underlying components.

class archetype {
public:
    archetype() = default;
    ~archetype() = default;

private:
    // Map from componentID to the components index in our collection
    // Type erased container around a component.
    struct component_wrapper {
        
    };

    std::unordered_map<component_id, size_t> _componentMap; // Map from componentid to their index in this specific archetype collection
};

}; // namespace gxe