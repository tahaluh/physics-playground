#pragma once

#include "engine/physics/3d/shapes/Shape3D.h"

class SphereShape3D : public Shape3D
{
public:
    explicit SphereShape3D(float radius)
        : radius(radius)
    {
    }

    float getRadius() const { return radius; }

private:
    float radius = 0.0f;
};
