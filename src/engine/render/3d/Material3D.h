#pragma once

#include <cstdint>

struct Material3D
{
    uint32_t fillColor = 0xFF3A86FF;
    uint32_t wireframeColor = 0xFF44AAFF;
    bool renderSolid = true;
    bool renderWireframe = true;
};
