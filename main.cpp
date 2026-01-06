#include <iostream>
#include <raylib.h>

#include "ecs/world.hpp"

[[maybe_unused]] const int SCREEN_WIDTH = 1200;
[[maybe_unused]] const int SCREEN_HEIGHT = 800;

constexpr size_t COLOR_COUNT = 21;
[[maybe_unused]] constexpr std::array<Color, COLOR_COUNT> colors{
    DARKGRAY, MAROON, ORANGE, DARKGREEN, DARKBLUE, DARKPURPLE, DARKBROWN,
    GRAY, RED, GOLD, LIME, BLUE, VIOLET, BROWN, LIGHTGRAY, PINK, YELLOW,
    GREEN, SKYBLUE, PURPLE, BEIGE
};

void init_raylib() {
    SetConfigFlags(FLAG_WINDOW_TOPMOST | FLAG_WINDOW_UNDECORATED);
    InitWindow(GetScreenWidth(), GetScreenWidth(), "CPU Render");
    SetTargetFPS(999);
}

struct position {
    float x{0};
    float y{0};
};

struct velocity {
    float vx{0};
    float vy{0};
};

int main(){
    std::cout << "Hello World" << std::endl;

#ifdef BUILD_DEBUG
    gxe::debug::run_ecs_tests();
#endif

    init_raylib();

    // Lets try creating entites with a position in the world, based on mouse position.
    RenderTexture circleTex = LoadRenderTexture(6, 6);
    BeginTextureMode(circleTex);
        DrawCircle(3, 3, 3.0f, WHITE);
    EndTextureMode();

    gxe::world world1;
    
    world1.register_component<position>();
    world1.register_component<velocity>();

    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(WHITE);

        if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
            Vector2 mPos = GetMousePosition();

            std::cout << "Created: " << world1.create_entity().add_component<position>(mPos.x, mPos.y).build() << " at " << mPos.x << ", " << mPos.y << "\n";
        }

        // Now, draw a circle at each position entity
        

        EndDrawing();
    }

    return 0;
}