#pragma once

#include <cstdint>

struct RenderMaterial
{
    uint32_t baseColor = 0xFFFFFFFF;
    float opacity = 1.0f;
    uint32_t emissiveColor = 0xFF000000;
    float ambientFactor = 1.0f;
    float diffuseFactor = 1.0f;
    float metallic = 0.0f;
    float roughness = 1.0f;
    bool unlit = false;
    bool doubleSidedLighting = false;

    uint32_t resolveBaseColor(uint32_t fallbackColor = 0) const
    {
        const uint32_t resolvedBaseColor = fallbackColor != 0 ? fallbackColor : baseColor;
        const uint32_t baseAlpha = (resolvedBaseColor >> 24) & 0xFF;
        const float clampedOpacity = opacity < 0.0f ? 0.0f : (opacity > 1.0f ? 1.0f : opacity);
        const uint32_t resolvedAlpha = static_cast<uint32_t>(static_cast<float>(baseAlpha) * clampedOpacity + 0.5f);
        return (resolvedBaseColor & 0x00FFFFFFu) | (resolvedAlpha << 24);
    }

    uint32_t resolveEmissiveColor() const
    {
        return emissiveColor;
    }

    bool isTransparent() const
    {
        return ((resolveBaseColor() >> 24) & 0xFF) < 255;
    }
};
