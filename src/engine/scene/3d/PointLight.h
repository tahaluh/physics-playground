#pragma once

#include <cstddef>
#include <cstdint>

#include "engine/math/Vector3.h"

struct PointLightDesc
{
    Vector3 position = Vector3::zero();
    uint32_t color = 0xFFFFFFFF;
    float intensity = 1.0f;
    float range = 12.0f;
    bool castShadows = true;
};

struct PointLight
{
    Vector3 position = Vector3::zero();
    uint32_t color = 0xFFFFFFFF;
    float intensity = 1.0f;
    float range = 12.0f;
    bool enabled = true;
    bool castShadows = true;
};

struct PointLightHandle
{
    std::size_t id = static_cast<std::size_t>(-1);

    bool isValid() const
    {
        return id != static_cast<std::size_t>(-1);
    }
};
