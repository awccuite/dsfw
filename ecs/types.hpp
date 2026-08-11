#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>
#include <atomic>

namespace gxe {

using entity_id = uint32_t;
using archetype_id = uint32_t;
using component_id = uint32_t;

constexpr inline entity_id NULL_ID = std::numeric_limits<entity_id>::max();
constexpr inline archetype_id NULL_ARCHETYPE_ID = std::numeric_limits<archetype_id>::max();

namespace detail {
// Consistent component ids across all world instances.
inline std::atomic<component_id> component_id_counter{0};
} // namespace detail

// Maps a component type to a stable id. The static local is instantiated once per type,
// so type <-> id is a bijection - which is what lets archetype cast a component_array_base
// back to component_array<C> without a runtime check.
template<typename C>
component_id get_component_id(){
    static const component_id id = detail::component_id_counter++;
    return id;
}

} // namespace gxe