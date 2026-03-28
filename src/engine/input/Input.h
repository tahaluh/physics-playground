#pragma once

#include <array>
#include <cstddef>

#include "engine/input/KeyCode.h"

class Input
{
public:
    static void beginFrame();
    static void setKeyState(EngineKeyCode key, bool isDown);
    static bool isKeyDown(EngineKeyCode key);
    static bool wasKeyPressed(EngineKeyCode key);

    static void setMousePosition(float x, float y);
    static void addMouseDelta(float dx, float dy);
    static void resetMouseDelta();
    static float getMouseX();
    static float getMouseY();
    static float getMouseDeltaX();
    static float getMouseDeltaY();

private:
    static inline std::array<bool, static_cast<std::size_t>(EngineKeyCode::Count)> currentKeys = {};
    static inline std::array<bool, static_cast<std::size_t>(EngineKeyCode::Count)> previousKeys = {};
    static inline float mouseX = 0.0f;
    static inline float mouseY = 0.0f;
    static inline float previousMouseX = 0.0f;
    static inline float previousMouseY = 0.0f;
    static inline float mouseDeltaX = 0.0f;
    static inline float mouseDeltaY = 0.0f;
};
