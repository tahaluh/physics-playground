#pragma once

#include "engine/math/Quaternion.h"
#include "engine/math/Vector3.h"

struct Transform
{
    Vector3 position = Vector3::zero();
    Quaternion rotation = Quaternion::identity();
    Vector3 scale = Vector3::one();
};
