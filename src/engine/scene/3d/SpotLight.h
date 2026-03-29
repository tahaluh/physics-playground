#pragma once

#include <cstddef>
#include <cstdint>

#include "engine/math/Vector3.h"

struct SpotLightDesc
{
    Vector3 position = Vector3::zero();
    Vector3 direction = Vector3(0.0f, -1.0f, 0.0f);
    uint32_t color = 0xFFFFFFFF;
    float intensity = 1.0f;
    float range = 12.0f;
    float innerConeCos = 0.95f;
    float outerConeCos = 0.8f;
};

struct SpotLight
{
    Vector3 position = Vector3::zero();
    Vector3 direction = Vector3(0.0f, -1.0f, 0.0f);
    uint32_t color = 0xFFFFFFFF;
    float intensity = 1.0f;
    float range = 12.0f;
    float innerConeCos = 0.95f;
    float outerConeCos = 0.8f;
    bool enabled = true;
};

struct SpotLightHandle
{
    std::size_t id = static_cast<std::size_t>(-1);

    bool isValid() const
    {
        return id != static_cast<std::size_t>(-1);
    }
};
