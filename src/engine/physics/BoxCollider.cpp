#include "engine/physics/BoxCollider.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "engine/physics/CollisionTests.h"
#include "engine/physics/PlaneCollider.h"

namespace
{
Vector3 componentMultiply(const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

Vector3 getWorldCenter(const BoxCollider &collider, const Transform &transform)
{
    const Vector3 scaledOffset = componentMultiply(collider.offset, transform.scale);
    const Matrix4 rotationMatrix = transform.rotation.toMatrix();
    return transform.position + rotationMatrix.transformVector(scaledOffset);
}

Vector3 getScaledHalfExtents(const BoxCollider &collider, const Transform &transform)
{
    return Vector3(
        std::abs(collider.halfExtents.x * transform.scale.x),
        std::abs(collider.halfExtents.y * transform.scale.y),
        std::abs(collider.halfExtents.z * transform.scale.z));
}

std::array<Vector3, 3> getBoxAxes(const Transform &transform)
{
    const Matrix4 rotationMatrix = transform.rotation.toMatrix();
    return {
        rotationMatrix.transformVector(Vector3::right()).normalized(),
        rotationMatrix.transformVector(Vector3::up()).normalized(),
        rotationMatrix.transformVector(Vector3::forward()).normalized()};
}
}

Collider::BroadPhaseBounds BoxCollider::getBroadPhaseBounds(const Transform &transform) const
{
    const Vector3 center = getWorldCenter(*this, transform);
    const Vector3 half = getScaledHalfExtents(*this, transform);
    const std::array<Vector3, 3> axes = getBoxAxes(transform);

    const Vector3 extents(
        std::abs(axes[0].x) * half.x + std::abs(axes[1].x) * half.y + std::abs(axes[2].x) * half.z,
        std::abs(axes[0].y) * half.x + std::abs(axes[1].y) * half.y + std::abs(axes[2].y) * half.z,
        std::abs(axes[0].z) * half.x + std::abs(axes[1].z) * half.y + std::abs(axes[2].z) * half.z);

    BroadPhaseBounds bounds;
    bounds.min = center - extents;
    bounds.max = center + extents;
    bounds.valid = true;
    return bounds;
}

CollisionPoints BoxCollider::testCollision(
    const Collider &other,
    const Transform &thisTransform,
    const Transform &otherTransform) const
{
    const auto *otherPlane = dynamic_cast<const PlaneCollider *>(&other);
    if (otherPlane)
    {
        return CollisionTests::testBoxPlane(*this, thisTransform, *otherPlane, otherTransform);
    }

    const auto *otherBox = dynamic_cast<const BoxCollider *>(&other);
    if (!otherBox)
    {
        return {};
    }
    return CollisionTests::testBoxBox(*this, thisTransform, *otherBox, otherTransform);
}

std::shared_ptr<Collider> BoxCollider::clone() const
{
    return std::make_shared<BoxCollider>(*this);
}
