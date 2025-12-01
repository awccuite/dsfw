#pragma once

#include <unordered_map>
#include <vector>

namespace gxe {

typedef size_t ComponentID;

// An archetype is a collection of components, that are related in that each of the
// entities that own one component in the archetype own one of every
// component in the archetype.

// Each archetype maintains a vector for each of its underlying components.

class Archetype {
public:
    Archetype() = default;
    ~Archetype() = default;

private:
    // Map from componentID to the components index in our collection
    // Type erased container around a component.
    struct ComponentWrapper {
    
    
    };

    std::unordered_map<ComponentID, size_t> _componentMap;
    
};

}; // namespace gxe