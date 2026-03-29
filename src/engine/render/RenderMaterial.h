#pragma once

#include <cstdint>

struct RenderMaterial
{
    uint32_t color = 0xFFFFFFFF;
    float opacity = 1.0f;

    uint32_t resolveColor(uint32_t fallbackColor = 0) const
    {
        const uint32_t baseColor = fallbackColor != 0 ? fallbackColor : color;
        const uint32_t baseAlpha = (baseColor >> 24) & 0xFF;
        const float clampedOpacity = opacity < 0.0f ? 0.0f : (opacity > 1.0f ? 1.0f : opacity);
        const uint32_t resolvedAlpha = static_cast<uint32_t>(static_cast<float>(baseAlpha) * clampedOpacity + 0.5f);
        return (baseColor & 0x00FFFFFFu) | (resolvedAlpha << 24);
    }

    bool isTransparent() const
    {
        return ((resolveColor() >> 24) & 0xFF) < 255;
    }
};
