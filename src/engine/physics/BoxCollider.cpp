#include "engine/physics/BoxCollider.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
constexpr float kAxisEpsilon = 0.00001f;

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

float projectBoxOntoAxis(
    const std::array<Vector3, 3> &axes,
    const Vector3 &halfExtents,
    const Vector3 &axis)
{
    return std::abs(axes[0].dot(axis)) * halfExtents.x +
           std::abs(axes[1].dot(axis)) * halfExtents.y +
           std::abs(axes[2].dot(axis)) * halfExtents.z;
}

Vector3 getSupportPoint(
    const Vector3 &center,
    const std::array<Vector3, 3> &axes,
    const Vector3 &halfExtents,
    const Vector3 &direction)
{
    Vector3 result = center;
    result += axes[0] * (axes[0].dot(direction) >= 0.0f ? halfExtents.x : -halfExtents.x);
    result += axes[1] * (axes[1].dot(direction) >= 0.0f ? halfExtents.y : -halfExtents.y);
    result += axes[2] * (axes[2].dot(direction) >= 0.0f ? halfExtents.z : -halfExtents.z);
    return result;
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
    const auto *otherBox = dynamic_cast<const BoxCollider *>(&other);
    if (!otherBox)
    {
        return {};
    }

    const Vector3 centerA = getWorldCenter(*this, thisTransform);
    const Vector3 centerB = getWorldCenter(*otherBox, otherTransform);
    const Vector3 halfA = getScaledHalfExtents(*this, thisTransform);
    const Vector3 halfB = getScaledHalfExtents(*otherBox, otherTransform);
    const std::array<Vector3, 3> axesA = getBoxAxes(thisTransform);
    const std::array<Vector3, 3> axesB = getBoxAxes(otherTransform);
    const Vector3 centerDelta = centerB - centerA;

    float minOverlap = std::numeric_limits<float>::max();
    Vector3 bestAxis = Vector3::zero();

    const auto testAxis = [&](const Vector3 &axisCandidate) -> bool {
        const float axisLengthSquared = axisCandidate.lengthSquared();
        if (axisLengthSquared <= kAxisEpsilon)
        {
            return true;
        }

        const Vector3 axis = axisCandidate / std::sqrt(axisLengthSquared);
        const float projectedCenterDistance = std::abs(centerDelta.dot(axis));
        const float projectedRadiusA = projectBoxOntoAxis(axesA, halfA, axis);
        const float projectedRadiusB = projectBoxOntoAxis(axesB, halfB, axis);
        const float overlap = projectedRadiusA + projectedRadiusB - projectedCenterDistance;
        if (overlap <= 0.0f)
        {
            return false;
        }

        if (overlap < minOverlap)
        {
            minOverlap = overlap;
            bestAxis = centerDelta.dot(axis) >= 0.0f ? axis : axis * -1.0f;
        }

        return true;
    };

    for (const Vector3 &axis : axesA)
    {
        if (!testAxis(axis))
        {
            return {};
        }
    }

    for (const Vector3 &axis : axesB)
    {
        if (!testAxis(axis))
        {
            return {};
        }
    }

    for (const Vector3 &axisA : axesA)
    {
        for (const Vector3 &axisB : axesB)
        {
            if (!testAxis(axisA.cross(axisB)))
            {
                return {};
            }
        }
    }

    CollisionPoints result;
    result.hasCollision = true;
    result.depth = minOverlap;
    result.normal = bestAxis;
    result.a = getSupportPoint(centerA, axesA, halfA, result.normal);
    result.b = getSupportPoint(centerB, axesB, halfB, result.normal * -1.0f);
    return result;
}

std::shared_ptr<Collider> BoxCollider::clone() const
{
    return std::make_shared<BoxCollider>(*this);
}
