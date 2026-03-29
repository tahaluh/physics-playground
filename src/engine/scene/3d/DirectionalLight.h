#pragma once

#include <cstdint>

#include "engine/math/Vector3.h"

struct DirectionalLightDesc
{
    Vector3 direction = Vector3(-0.3f, -1.0f, -0.2f).normalized();
    uint32_t color = 0xFFFFFFFF;
    float intensity = 1.0f;
};

struct DirectionalLight
{
    Vector3 direction = Vector3(-0.3f, -1.0f, -0.2f).normalized();
    uint32_t color = 0xFFFFFFFF;
    float intensity = 1.0f;
    bool enabled = true;
};

struct DirectionalLightHandle
{
    std::size_t id = static_cast<std::size_t>(-1);

    bool isValid() const
    {
        return id != static_cast<std::size_t>(-1);
    }
};
