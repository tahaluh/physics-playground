#pragma once

#include <array>

#include "math/Vector2.h"

class PhysicsBody2D;

struct Manifold2D
{
    PhysicsBody2D *bodyA = nullptr;
    PhysicsBody2D *bodyB = nullptr;
    Vector2 normal = Vector2::zero();
    float penetration = 0.0f;
    std::array<Vector2, 2> contactPoints = {Vector2::zero(), Vector2::zero()};
    size_t contactCount = 0;
};
