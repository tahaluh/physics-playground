#include "engine/input/Input.h"

#include <cstddef>

void Input::beginFrame()
{
    previousKeys = currentKeys;
    previousMouseX = mouseX;
    previousMouseY = mouseY;
    mouseDeltaX = 0.0f;
    mouseDeltaY = 0.0f;
}

void Input::setKeyState(EngineKeyCode key, bool isDown)
{
    currentKeys[static_cast<std::size_t>(key)] = isDown;
}

bool Input::isKeyDown(EngineKeyCode key)
{
    return currentKeys[static_cast<std::size_t>(key)];
}

bool Input::wasKeyPressed(EngineKeyCode key)
{
    const std::size_t index = static_cast<std::size_t>(key);
    return currentKeys[index] && !previousKeys[index];
}

void Input::setMousePosition(float x, float y)
{
    mouseX = x;
    mouseY = y;
}

void Input::addMouseDelta(float dx, float dy)
{
    mouseDeltaX += dx;
    mouseDeltaY += dy;
}

void Input::resetMouseDelta()
{
    mouseDeltaX = 0.0f;
    mouseDeltaY = 0.0f;
}

float Input::getMouseX()
{
    return mouseX;
}

float Input::getMouseY()
{
    return mouseY;
}

float Input::getMouseDeltaX()
{
    return mouseDeltaX;
}

float Input::getMouseDeltaY()
{
    return mouseDeltaY;
}
