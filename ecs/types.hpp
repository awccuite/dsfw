#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>

namespace gxe {

using entityid = uint32_t;
using archetypeid = entityid;

constexpr inline entityid NULL_ID = std::numeric_limits<entityid>::max();
constexpr inline archetypeid NULL_ARCHETYPE_ID = std::numeric_limits<archetypeid>::max();

constexpr inline std::size_t INITIAL_SPARSE_SET_CAPACITY = 1024;

// Type-erased base class for ECS to allow archetype -> ecs lookups
class ecs_base {
public:
    virtual ~ecs_base() = default;
    
    // Get the archetype-local index for an entity
    virtual archetypeid getArchetypeLocalId(entityid id) const = 0;
    
    // Update the archetype-local index for an entity (called during swap-and-pop)
    virtual void setArchetypeLocalId(entityid id, archetypeid localId) = 0;
};

// Example component types
struct Position {
    float x, y;
};

struct Velocity { // Needs to be units/sec
    float dx, dy;
};

struct Lifetime {
    float ttl;
};

struct EColor {
    size_t col;
};

struct AABB {
    float xmin, xmax, ymin, ymax;
};

struct Circle {
    float radius, xpos, ypos;
};

} // namespace gxe