#pragma once

#include "engine/math/Vector2.h"

struct ProjectedVertex3D
{
    Vector2 position = Vector2::zero();
    float depth = 0.0f;
    bool visible = false;
};
