#pragma once

#include <functional>
#include <optional>
#include <iostream>

// Systems can be created to update at some frequency T,
// or triggered manually.

// Systems should be templated over the components they access, and should maintain
// a pointer to the ECS.

// Systems should be archetype agnostic, instead utilizing components.
// Base class handling tick rate and time accumulation

namespace gxe {

class system_base {
public:
    system_base(uint32_t tickrate):
        _tickrate(tickrate),
        _accumulatedTime(0.0f),
        _secsPerTick(tickrate > 0 ? 1.0f / tickrate : 0.0f) {};

    virtual ~system_base() = default;

    // Virtual update for polymorphic calls through base pointer
    void update(float dt) {
        if (_tickrate == 0) {
            tick(dt);
            return;
        }

        _accumulatedTime += dt;

        while (_accumulatedTime >= _secsPerTick) {
            tick(_secsPerTick);
            _accumulatedTime -= _secsPerTick;
        }
    }

    uint32_t tickrate() const { return _tickrate; }

protected:
    virtual void tick(float dt) = 0;

    const uint32_t _tickrate;
    float _accumulatedTime;
    float _secsPerTick;
};

// CRTP base for custom systems - derive from this to create your own system types
// Usage:
//   class MyPhysicsSystem : public SystemCRTP<MyPhysicsSystem, ECSType> {
//   public:
//       MyPhysicsSystem(ECSType& ecs) : SystemCRTP(60, ecs) {}
//       void tick(float dt) { /* custom logic using _ecs */ }
//   };
template <typename Derived, typename ECS>
class SystemCRTP : public system_base {
public:
    SystemCRTP(ECS& world, uint32_t tickrate = 0) // Default of update as frequently as possible
        : system_base(tickrate), _world(world) {}

protected:
    ECS& _world; // Reference to the world with which this system operates in
};

// Lambda-based system for quick inline definitions (original behavior)
class LambdaSystem : public system_base {
public:
    LambdaSystem(uint32_t tickrate) : system_base(tickrate) {};
    ~LambdaSystem() = default;

    // Set tick to some function F.
    template<typename F>
    void tickDef(F&& lambda){ 
        _tickImpl = [lambda = std::forward<F>(lambda)](float dt) mutable {
            if constexpr (std::is_invocable_v<F, float>) {
                lambda(dt);
            } else {
                lambda();
            }
        };
    }

protected:
    void tick(float sysDelta) override {
        if(_tickImpl){
            (*_tickImpl)(sysDelta);
        }
    }

private:
    std::optional<std::function<void(float)>> _tickImpl;
};

}; // Namespace gxe