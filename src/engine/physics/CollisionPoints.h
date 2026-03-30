#pragma once

#include "engine/math/Vector3.h"

struct CollisionPoints
{
    Vector3 a = Vector3::zero();
    Vector3 b = Vector3::zero();
    Vector3 normal = Vector3::zero();
    float depth = 0.0f;
    bool hasCollision = false;
};
