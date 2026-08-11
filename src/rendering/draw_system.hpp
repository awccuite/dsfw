#pragma once

#include <raylib.h>
#include <span>

#include "ecs/systems/component_system.hpp"
#include "src/physics/particle_system.hpp"

namespace render {

// Owns the window and the raylib frame. Because BeginDrawing/ClearBackground/EndDrawing
// all live in tick, a skipped tick draws nothing AND clears nothing, so there is no blank
// frame to flicker. EndDrawing is also where raylib polls input and applies frame pacing,
// which is why this is UNCAPPED - it must run exactly once per advance.
//
// Window, input and rendering are one system here because raylib fuses them: EndDrawing
// both presents and polls input. On GLFW/SDL3 these separate cleanly and this splits into
// window_system, input_system and draw_system, all UNCAPPED, registered in that order.
class draw_system
    : public gxe::systems::component_system<draw_system,
                                            gxe::systems::UPDATE_TYPE::UNCAPPED,
                                            phys::position, phys::color_index> {
public:
    static constexpr int k_circle_size = 6;
    static constexpr int k_target_fps = 999;

    draw_system(int width, int height, std::span<const Color> palette,
                const char* title = "CPU Render", Color background = WHITE)
        : component_system(0),
          _palette(palette),
          _background(background) {
        SetConfigFlags(FLAG_WINDOW_TOPMOST | FLAG_WINDOW_UNDECORATED);
        InitWindow(width, height, title);
        SetTargetFPS(k_target_fps);

        _texture = LoadRenderTexture(k_circle_size, k_circle_size);
        BeginTextureMode(_texture);
            DrawCircle(k_circle_size / 2, k_circle_size / 2,
                       static_cast<float>(k_circle_size) / 2.0f, WHITE);
        EndTextureMode();

        const float w = static_cast<float>(_texture.texture.width);
        const float h = static_cast<float>(_texture.texture.height);

        _source = Rectangle{0.0f, 0.0f, w, -h}; // negative height undoes the render texture flip
        _halfWidth = w / 2.0f;
        _halfHeight = h / 2.0f;
    }

    // Texture first: CloseWindow destroys the GL context the texture lives in.
    ~draw_system() override {
        UnloadRenderTexture(_texture);
        CloseWindow();
    }

    draw_system(const draw_system&) = delete;
    draw_system& operator=(const draw_system&) = delete;

    bool should_close() const { return WindowShouldClose(); }

    void tick(gxe::world& w) override {
        BeginDrawing();
        ClearBackground(_background);

        _drawn = 0;
        component_system::tick(w);

        draw_overlay();
        EndDrawing();
    }

    void update(phys::position& p, phys::color_index& idx) {
        DrawTextureRec(_texture.texture, _source,
                       Vector2{p.x.to_float() - _halfWidth, p.y.to_float() - _halfHeight},
                       _palette[static_cast<size_t>(idx.idx)]);
        _drawn++;
    }

private:
    // --- TEMPORARY PERF READOUT (remove once numbers are collected) ---
    void draw_overlay() {
        DrawFPS(10, 10);
        DrawText(TextFormat("entities: %i", static_cast<int>(_drawn)), 10, 34, 20, BLACK);
    }
    // --- END TEMPORARY ---

    RenderTexture _texture{};
    std::span<const Color> _palette;
    Color _background;
    Rectangle _source{};
    float _halfWidth{0.0f};
    float _halfHeight{0.0f};
    size_t _drawn{0};
};

} // namespace render
