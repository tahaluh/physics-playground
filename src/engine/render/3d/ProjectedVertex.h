#pragma once

#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"

struct ProjectedVertex
{
    Vector2 position = Vector2::zero();
    Vector3 worldPosition = Vector3::zero();
    Vector3 viewPosition = Vector3::zero();
    float depth = 0.0f;
    bool visible = false;
};
