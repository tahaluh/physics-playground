#pragma once

#include "engine/render/RenderMaterial.h"

struct Material3D
{
    RenderMaterial solid = {0xFF3A86FF, 1.0f, 0xFF000000, 1.0f, 1.0f, 0.0f, 1.0f, false, false};
    RenderMaterial wireframe = {0xFF44AAFF, 1.0f, 0xFF000000, 1.0f, 0.0f, 0.0f, 1.0f, true, false};
    bool renderSolid = true;
    bool renderWireframe = true;

    bool isTransparent() const
    {
        return (renderSolid && solid.isTransparent()) ||
               (renderWireframe && wireframe.isTransparent());
    }
};

namespace MaterialPresets3D
{
inline RenderMaterial makePlastic(uint32_t baseColor, float roughness = 0.45f)
{
    RenderMaterial material;
    material.baseColor = baseColor;
    material.metallic = 0.0f;
    material.roughness = roughness;
    return material;
}

inline RenderMaterial makeRubber(uint32_t baseColor)
{
    RenderMaterial material;
    material.baseColor = baseColor;
    material.metallic = 0.0f;
    material.roughness = 0.9f;
    material.diffuseFactor = 0.95f;
    return material;
}

inline RenderMaterial makeBrushedSteel(uint32_t baseColor = 0xFFC8CDD3)
{
    RenderMaterial material;
    material.baseColor = baseColor;
    material.metallic = 1.0f;
    material.roughness = 0.38f;
    return material;
}

inline RenderMaterial makeCopper(uint32_t baseColor = 0xFFB87333)
{
    RenderMaterial material;
    material.baseColor = baseColor;
    material.metallic = 1.0f;
    material.roughness = 0.28f;
    return material;
}

inline RenderMaterial makeFrostedGlass(uint32_t baseColor = 0xE6DFF7FF)
{
    RenderMaterial material;
    material.baseColor = baseColor;
    material.opacity = 0.35f;
    material.metallic = 0.0f;
    material.roughness = 0.08f;
    material.diffuseFactor = 0.15f;
    material.doubleSidedLighting = true;
    return material;
}

inline RenderMaterial makeStone(uint32_t baseColor = 0xFF8C8A85)
{
    RenderMaterial material;
    material.baseColor = baseColor;
    material.metallic = 0.0f;
    material.roughness = 0.95f;
    material.diffuseFactor = 1.0f;
    return material;
}
}
