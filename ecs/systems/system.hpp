#pragma once

#include <chrono>
#include <cstdint>

namespace gxe {

class world;

} // namespace gxe

namespace gxe::systems {

enum class UPDATE_TYPE {
    FIXED,    // hz is a target: runs every owed tick, so sim time keeps up with wall time
    CAPPED,   // hz is a ceiling: at most one tick per advance call
    UNCAPPED, // exactly one tick per advance call; hz is ignored
};

// Type-erased handle so world can hold systems of differing update types.
class system_base {
public:
    // hz 0 means uncapped and resolves to k_max_hz, as does any rate above it.
    static constexpr uint32_t k_max_hz = 999;

    virtual ~system_base() = default;

    virtual void advance(world& w, std::chrono::nanoseconds elapsed) = 0;
    virtual void tick(world& w) = 0;

    // Called by world::register_system. Systems declare the components they need
    // rather than the caller registering them beforehand.
    virtual void register_components(world&) {}

    uint32_t hz() const { return _hz; }

protected:
    explicit system_base(uint32_t hz)
        : _hz((hz == 0 || hz > k_max_hz) ? k_max_hz : hz) {}

    static constexpr int64_t k_nano = 1'000'000'000;

    uint32_t _hz;
    int64_t  _accumulated{0};
};

// Systems derive from this (or from component_system, which derives from it) and
// implement tick. The update type is part of the type, so advance specializes on it
// with no runtime state or branching.
template<UPDATE_TYPE Type>
class system : public system_base {
public:
    static constexpr UPDATE_TYPE k_update_type = Type;

    using system_base::system_base;

    void advance(world& w, std::chrono::nanoseconds elapsed) final {
        if constexpr (Type == UPDATE_TYPE::UNCAPPED) {
            tick(w);
        } else {
            // Scaling nanoseconds by hz keeps the tick count exact and independent of
            // how the elapsed time was split into frames.
            _accumulated += elapsed.count() * static_cast<int64_t>(_hz);

            if constexpr (Type == UPDATE_TYPE::CAPPED) {
                if (_accumulated >= k_nano) {
                    _accumulated = 0; // drop the remainder; a ceiling must not build a backlog
                    tick(w);
                }
            } else {
                while (_accumulated >= k_nano) {
                    _accumulated -= k_nano;
                    tick(w);
                }
            }
        }
    }
};

} // namespace gxe::systems
