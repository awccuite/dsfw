#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>

namespace gxe {

using entityid = uint32_t;
constexpr inline entityid NULL_ID = std::numeric_limits<entityid>::max();

constexpr inline std::size_t INITIAL_SPARSE_SET_CAPACITY = 1024;

} // namespace gxeE