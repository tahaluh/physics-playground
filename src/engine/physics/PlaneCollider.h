#pragma once

#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"
#include "engine/physics/Collider.h"

class PlaneCollider : public Collider
{
public:
    Vector3 offset = Vector3::zero();
    Vector2 halfSize = Vector2(0.5f, 0.5f);

    PlaneCollider() = default;
    PlaneCollider(const Vector3 &offset, const Vector2 &halfSize)
        : offset(offset), halfSize(halfSize) {}

    CollisionPoints testCollision(
        const Collider &other,
        const Transform &thisTransform,
        const Transform &otherTransform) const override;

    BroadPhaseBounds getBroadPhaseBounds(const Transform &transform) const override;
    std::shared_ptr<Collider> clone() const override;
};
