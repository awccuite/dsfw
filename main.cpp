#include <iostream>
#include <array>
#include <chrono>

#include "src/rendering/draw_system.hpp"

const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;

constexpr size_t COLOR_COUNT = 21;
constexpr std::array<Color, COLOR_COUNT> colors{
    DARKGRAY, MAROON, ORANGE, DARKGREEN, DARKBLUE, DARKPURPLE, DARKBROWN,
    GRAY, RED, GOLD, LIME, BLUE, VIOLET, BROWN, LIGHTGRAY, PINK, YELLOW,
    GREEN, SKYBLUE, PURPLE, BEIGE
};

int main(){
    std::cout << "Hello World" << std::endl;

#ifdef BUILD_DEBUG
    gxe::debug::run_ecs_tests();
#endif

    gxe::world world1;

    auto& particles = world1.register_system<phys::particle_system>(
        phys::fixed::from_int(SCREEN_WIDTH),
        phys::fixed::from_int(SCREEN_HEIGHT),
        static_cast<uint32_t>(COLOR_COUNT));

    // Constructing this opens the window, so nothing raylib may run before it.
    auto& renderer = world1.register_system<render::draw_system>(
        SCREEN_WIDTH, SCREEN_HEIGHT, colors);

    auto last = std::chrono::steady_clock::now();

    while(!renderer.should_close()){
        const auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last);
        last = now;

        // Admit at most 250ms of wall time per frame so a long stall cannot cascade
        // into a tick backlog that takes longer to run than the time it represents.
        if(elapsed > std::chrono::milliseconds(250)) elapsed = std::chrono::milliseconds(250);

        if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
            Vector2 mPos = GetMousePosition();

            particles.spawn(world1,
                            phys::fixed::from_float(mPos.x),
                            phys::fixed::from_float(mPos.y));
        }

        world1.step(elapsed);
    }

    return 0;
}
