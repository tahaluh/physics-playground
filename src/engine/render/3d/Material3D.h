#pragma once

#include "engine/render/RenderMaterial.h"

struct Material3D
{
    RenderMaterial solid = {0xFF3A86FF, 1.0f, 0xFF000000, 1.0f, 1.0f, 0.0f, 16.0f, 0.0f, 1.0f};
    RenderMaterial wireframe = {0xFF44AAFF, 1.0f, 0xFF000000, 1.0f, 0.0f, 0.0f, 16.0f, 0.0f, 1.0f};
    bool renderSolid = true;
    bool renderWireframe = true;

    bool isTransparent() const
    {
        return (renderSolid && solid.isTransparent()) ||
               (renderWireframe && wireframe.isTransparent());
    }
};
