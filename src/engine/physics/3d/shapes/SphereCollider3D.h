#pragma once

#include "engine/physics/3d/shapes/Collider3D.h"

class SphereCollider3D : public Collider3D
{
public:
    explicit SphereCollider3D(float radius)
        : radius(radius)
    {
    }

    float getRadius() const { return radius; }

private:
    float radius = 0.0f;
};
