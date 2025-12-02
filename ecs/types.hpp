#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>

namespace gxe {

using entity_id = uint32_t;
using archetype_id = uint32_t;
using component_id = uint32_t;

constexpr inline entity_id NULL_ID = std::numeric_limits<entity_id>::max();
constexpr inline archetype_id NULL_ARCHETYPE_ID = std::numeric_limits<archetype_id>::max();

} // namespace gxe