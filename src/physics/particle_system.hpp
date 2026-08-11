#pragma once

#include "src/physics/fixed.hpp"
#include "ecs/systems/component_system.hpp"

#include <array>
#include <cstdint>
#include <random>
#include <utility>

namespace phys {

// Namespace scope, not a member: a consteval member function called from a static
// constexpr member initializer of the same class segfaults Apple clang 17. The
// initializer is parsed while the class is still incomplete, so the function body
// does not exist yet.
template<int32_t Min, int32_t Max, int32_t Step>
consteval auto speed_table() {
    constexpr size_t n = static_cast<size_t>((Max - Min) / Step) + 1;
    std::array<fixed, n> table{};

    for (size_t i = 0; i < n; i++) {
        table[i] = fixed::from_int(Min + static_cast<int32_t>(i) * Step);
    }

    return table;
}

struct position { fixed x, y; };
struct velocity { fixed magnitude, dx, dy; };  // dx,dy unit vector; magnitude in units/second
struct hitbox2d { fixed halfWidth, halfHeight; };
struct color_index { int32_t idx; };

class particle_system
    : public gxe::systems::component_system<particle_system,
                                            gxe::systems::UPDATE_TYPE::FIXED,
                                            position, velocity, hitbox2d> {
public:
    static constexpr uint32_t k_hz = 60;

    static constexpr int32_t k_speed_min_per_sec  = 60;
    static constexpr int32_t k_speed_max_per_sec  = 600;
    static constexpr int32_t k_speed_step_per_sec = 60;

    static constexpr int32_t  k_particle_radius = 3;
    static constexpr uint32_t k_default_seed    = 0x9E3779B9u;

    // Half-extent of the square directions are rejection-sampled from.
    static constexpr int32_t k_direction_extent = 1 << 15;

    particle_system(fixed world_width, fixed world_height, uint32_t color_count,
                    uint32_t seed = k_default_seed)
        : component_system(k_hz),
          _width(world_width),
          _height(world_height),
          _colorCount(color_count),
          _rng(seed) {}

    // color_index is written by spawn but not read by update, so it is not in the
    // query pack and has to be registered on top of it.
    void register_components(gxe::world& w) override {
        component_system::register_components(w);
        w.register_component<color_index>();
    }

    gxe::entity_id spawn(gxe::world& w, fixed x, fixed y) {
        const auto [dx, dy] = random_direction();
        const fixed magnitude = k_speeds[bounded(static_cast<uint32_t>(k_speeds.size()))];
        const fixed radius = fixed::from_int(k_particle_radius);

        return w.spawn_entity(
            position{x, y},
            velocity{magnitude, dx, dy},
            hitbox2d{radius, radius},
            color_index{static_cast<int32_t>(bounded(_colorCount))});
    }

    void update(position& p, velocity& v, hitbox2d& hb) {
        p.x += (v.dx * v.magnitude) / static_cast<int64_t>(k_hz);
        p.y += (v.dy * v.magnitude) / static_cast<int64_t>(k_hz);

        if (p.x - hb.halfWidth < fixed{}) {
            p.x = hb.halfWidth;
            v.dx = -v.dx;
        } else if (p.x + hb.halfWidth > _width) {
            p.x = _width - hb.halfWidth;
            v.dx = -v.dx;
        }

        if (p.y - hb.halfHeight < fixed{}) {
            p.y = hb.halfHeight;
            v.dy = -v.dy;
        } else if (p.y + hb.halfHeight > _height) {
            p.y = _height - hb.halfHeight;
            v.dy = -v.dy;
        }
    }

private:
    static constexpr auto k_speeds =
        speed_table<k_speed_min_per_sec, k_speed_max_per_sec, k_speed_step_per_sec>();

    // std::uniform_int_distribution is not portable across standard library
    // implementations - only mt19937 itself is specified exactly. This is unbiased
    // rejection sampling over raw engine output, so it reproduces anywhere.
    uint32_t bounded(uint32_t n) {
        const uint32_t threshold = static_cast<uint32_t>(-n) % n;

        for (;;) {
            const uint32_t r = static_cast<uint32_t>(_rng());
            if (r >= threshold) return r % n;
        }
    }

    // Rejection-sample the disc, then normalize with an integer sqrt. Avoids sin/cos,
    // which are not correctly rounded and differ between libm implementations.
    std::pair<fixed, fixed> random_direction() {
        const uint32_t span = 2u * static_cast<uint32_t>(k_direction_extent) + 1u;

        int32_t ix = 0;
        int32_t iy = 0;
        int64_t d2 = 0;

        do {
            ix = static_cast<int32_t>(bounded(span)) - k_direction_extent;
            iy = static_cast<int32_t>(bounded(span)) - k_direction_extent;
            d2 = static_cast<int64_t>(ix) * ix + static_cast<int64_t>(iy) * iy;
        } while (d2 > static_cast<int64_t>(k_direction_extent) * k_direction_extent || d2 == 0);

        const auto len = static_cast<int64_t>(isqrt(static_cast<unsigned __int128>(d2)));

        return {
            fixed::from_raw((static_cast<int64_t>(ix) << fixed::k_frac_bits) / len),
            fixed::from_raw((static_cast<int64_t>(iy) << fixed::k_frac_bits) / len)
        };
    }

    fixed _width;
    fixed _height;
    uint32_t _colorCount;
    std::mt19937 _rng;
};

} // namespace phys
