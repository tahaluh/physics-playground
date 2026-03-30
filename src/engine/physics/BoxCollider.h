#pragma once

#include "engine/math/Vector3.h"
#include "engine/physics/Collider.h"

class BoxCollider : public Collider
{
public:
    Vector3 offset = Vector3::zero();
    Vector3 halfExtents = Vector3::one() * 0.5f;

    BoxCollider() = default;
    BoxCollider(const Vector3 &offset, const Vector3 &halfExtents)
        : offset(offset), halfExtents(halfExtents) {}

    CollisionPoints testCollision(
        const Collider &other,
        const Transform &thisTransform,
        const Transform &otherTransform) const override;

    BroadPhaseBounds getBroadPhaseBounds(const Transform &transform) const override;
    std::shared_ptr<Collider> clone() const override;
};
