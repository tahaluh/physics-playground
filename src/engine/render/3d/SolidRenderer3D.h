#pragma once

#include <cstdint>

#include "engine/math/Transform3D.h"

class Camera3D;
class Renderer2D;

class SolidRenderer3D
{
public:
    void drawCube(Renderer2D &renderer, const Camera3D &camera, const Transform3D &transform, float size) const;
};
