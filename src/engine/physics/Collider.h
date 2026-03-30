#pragma once

#include <memory>

#include "engine/math/Transform.h"
#include "engine/physics/CollisionPoints.h"

class Collider
{
public:
    struct BroadPhaseBounds
    {
        Vector3 min = Vector3::zero();
        Vector3 max = Vector3::zero();
        bool valid = false;
    };

    virtual ~Collider() = default;

    virtual CollisionPoints testCollision(
        const Collider &other,
        const Transform &thisTransform,
        const Transform &otherTransform) const = 0;

    virtual BroadPhaseBounds getBroadPhaseBounds(const Transform &transform) const = 0;
    virtual std::shared_ptr<Collider> clone() const = 0;
};
