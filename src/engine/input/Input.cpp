#include "engine/input/Input.h"

#include <cstddef>

namespace
{
bool isActionMappedDown(EngineInputAction action, const std::array<bool, static_cast<std::size_t>(EngineKeyCode::Count)> &keys)
{
    switch (action)
    {
    case EngineInputAction::MoveForward:
        return keys[static_cast<std::size_t>(EngineKeyCode::W)];
    case EngineInputAction::MoveBackward:
        return keys[static_cast<std::size_t>(EngineKeyCode::S)];
    case EngineInputAction::MoveLeft:
        return keys[static_cast<std::size_t>(EngineKeyCode::A)];
    case EngineInputAction::MoveRight:
        return keys[static_cast<std::size_t>(EngineKeyCode::D)];
    case EngineInputAction::MoveUp:
        return keys[static_cast<std::size_t>(EngineKeyCode::E)];
    case EngineInputAction::MoveDown:
        return keys[static_cast<std::size_t>(EngineKeyCode::Q)];
    case EngineInputAction::ToggleLightDebug:
        return keys[static_cast<std::size_t>(EngineKeyCode::F1)];
    case EngineInputAction::ToggleWireframe:
        return keys[static_cast<std::size_t>(EngineKeyCode::F2)];
    case EngineInputAction::TogglePhysicsDebug:
        return keys[static_cast<std::size_t>(EngineKeyCode::F3)];
    default:
        return false;
    }
}
}

void Input::beginFrame()
{
    previousKeys = currentKeys;
    pressedKeys.fill(false);
    previousMouseX = mouseX;
    previousMouseY = mouseY;
    mouseDeltaX = 0.0f;
    mouseDeltaY = 0.0f;
}

void Input::setKeyState(EngineKeyCode key, bool isDown)
{
    const std::size_t index = static_cast<std::size_t>(key);
    if (isDown && !currentKeys[index])
    {
        pressedKeys[index] = true;
    }

    currentKeys[index] = isDown;
}

bool Input::isKeyDown(EngineKeyCode key)
{
    return currentKeys[static_cast<std::size_t>(key)];
}

bool Input::wasKeyPressed(EngineKeyCode key)
{
    return pressedKeys[static_cast<std::size_t>(key)];
}

bool Input::isActionDown(EngineInputAction action)
{
    return isActionMappedDown(action, currentKeys);
}

bool Input::wasActionPressed(EngineInputAction action)
{
    switch (action)
    {
    case EngineInputAction::ToggleLightDebug:
        return pressedKeys[static_cast<std::size_t>(EngineKeyCode::F1)];
    case EngineInputAction::ToggleWireframe:
        return pressedKeys[static_cast<std::size_t>(EngineKeyCode::F2)];
    case EngineInputAction::TogglePhysicsDebug:
        return pressedKeys[static_cast<std::size_t>(EngineKeyCode::F3)];
    default:
        return isActionDown(action);
    }
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
