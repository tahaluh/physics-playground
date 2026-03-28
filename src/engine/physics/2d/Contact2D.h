#pragma once

#include "engine/math/Vector2.h"

class PhysicsBody2D;

struct Contact2D
{
    PhysicsBody2D *self = nullptr;
    PhysicsBody2D *other = nullptr;
    Vector2 normal = Vector2::zero();
    float penetration = 0.0f;
    Vector2 contactPoint = Vector2::zero();
};
