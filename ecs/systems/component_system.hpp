#pragma once

#include "ecs/world.hpp"

namespace gxe::systems {

// Declares the components a system operates on and lets the world drive iteration.
// Derived supplies update(Components&...), called once per matching entity per tick.
//
// Systems needing several queries per tick, or work that is not per-entity, derive
// from system<Type> directly and implement tick() themselves.
template<typename Derived, UPDATE_TYPE Type, typename ...Components>
class component_system : public system<Type> {
public:
    using system<Type>::system;

    // Not final: a system that brackets its query with per-tick setup or teardown
    // overrides this and calls it in the middle.
    void tick(world& w) override {
        w.for_each_with_components<Components...>([this](Components&... components){
            static_cast<Derived*>(this)->update(components...);
        });
    }

    // Not final either: a system that also writes components outside its query pack
    // overrides this, calls it, and registers the extras.
    void register_components(world& w) override {
        (w.register_component<Components>(), ...);
    }
};

} // namespace gxe::systems
