#pragma once

#include "engine/math/Vector3.h"
#include "engine/physics/3d/shapes/Collider3D.h"

class BoxCollider3D : public Collider3D
{
public:
    explicit BoxCollider3D(const Vector3 &halfExtents)
        : halfExtents(halfExtents)
    {
    }

    const Vector3 &getHalfExtents() const { return halfExtents; }

private:
    Vector3 halfExtents = Vector3::one() * 0.5f;
};
