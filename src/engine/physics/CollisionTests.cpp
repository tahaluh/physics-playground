#include "engine/physics/CollisionTests.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "engine/math/Matrix4.h"
#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"
#include "engine/physics/BoxCollider.h"
#include "engine/physics/PlaneCollider.h"

namespace
{
constexpr float kAxisEpsilon = 0.00001f;

Vector3 componentMultiply(const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

Vector3 getBoxWorldCenter(const BoxCollider &collider, const Transform &transform)
{
    const Vector3 scaledOffset = componentMultiply(collider.offset, transform.scale);
    const Matrix4 rotationMatrix = transform.rotation.toMatrix();
    return transform.position + rotationMatrix.transformVector(scaledOffset);
}

Vector3 getBoxScaledHalfExtents(const BoxCollider &collider, const Transform &transform)
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

void getPlaneBasis(
    const PlaneCollider &collider,
    const Transform &transform,
    Vector3 &center,
    Vector3 &axisU,
    Vector3 &axisV,
    Vector3 &normal,
    Vector2 &scaledHalfSize)
{
    const Matrix4 rotationMatrix = transform.rotation.toMatrix();
    center = transform.position + rotationMatrix.transformVector(componentMultiply(collider.offset, transform.scale));
    axisU = rotationMatrix.transformVector(Vector3::right()).normalized();
    axisV = rotationMatrix.transformVector(Vector3::forward()).normalized();
    normal = rotationMatrix.transformVector(Vector3::up()).normalized();
    scaledHalfSize = Vector2(
        std::abs(collider.halfSize.x * transform.scale.x),
        std::abs(collider.halfSize.y * transform.scale.z));
}

std::array<Vector3, 8> getBoxVertices(const BoxCollider &collider, const Transform &transform)
{
    const Matrix4 rotationMatrix = transform.rotation.toMatrix();
    const Vector3 center = getBoxWorldCenter(collider, transform);
    const Vector3 halfExtents = getBoxScaledHalfExtents(collider, transform);
    const Vector3 axisX = rotationMatrix.transformVector(Vector3::right()).normalized();
    const Vector3 axisY = rotationMatrix.transformVector(Vector3::up()).normalized();
    const Vector3 axisZ = rotationMatrix.transformVector(Vector3::forward()).normalized();

    std::array<Vector3, 8> vertices{};
    std::size_t index = 0;
    for (int xSign : {-1, 1})
    {
        for (int ySign : {-1, 1})
        {
            for (int zSign : {-1, 1})
            {
                vertices[index++] =
                    center +
                    axisX * (halfExtents.x * static_cast<float>(xSign)) +
                    axisY * (halfExtents.y * static_cast<float>(ySign)) +
                    axisZ * (halfExtents.z * static_cast<float>(zSign));
            }
        }
    }

    return vertices;
}
}

CollisionPoints CollisionTests::testBoxBox(
    const BoxCollider &boxA,
    const Transform &transformA,
    const BoxCollider &boxB,
    const Transform &transformB)
{
    const Vector3 centerA = getBoxWorldCenter(boxA, transformA);
    const Vector3 centerB = getBoxWorldCenter(boxB, transformB);
    const Vector3 halfA = getBoxScaledHalfExtents(boxA, transformA);
    const Vector3 halfB = getBoxScaledHalfExtents(boxB, transformB);
    const std::array<Vector3, 3> axesA = getBoxAxes(transformA);
    const std::array<Vector3, 3> axesB = getBoxAxes(transformB);
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

CollisionPoints CollisionTests::testBoxPlane(
    const BoxCollider &box,
    const Transform &boxTransform,
    const PlaneCollider &plane,
    const Transform &planeTransform)
{
    const std::vector<CollisionPoints> manifold = testBoxPlaneContactManifold(box, boxTransform, plane, planeTransform);
    if (manifold.empty())
    {
        return {};
    }

    CollisionPoints deepest = manifold.front();
    for (const CollisionPoints &point : manifold)
    {
        if (point.depth > deepest.depth)
        {
            deepest = point;
        }
    }

    return deepest;
}

std::vector<CollisionPoints> CollisionTests::testBoxPlaneContactManifold(
    const BoxCollider &box,
    const Transform &boxTransform,
    const PlaneCollider &plane,
    const Transform &planeTransform)
{
    Vector3 planeCenter;
    Vector3 planeAxisU;
    Vector3 planeAxisV;
    Vector3 planeNormal;
    Vector2 planeHalfSize;
    getPlaneBasis(plane, planeTransform, planeCenter, planeAxisU, planeAxisV, planeNormal, planeHalfSize);

    const std::array<Vector3, 8> boxVertices = getBoxVertices(box, boxTransform);
    std::vector<CollisionPoints> contacts;
    contacts.reserve(boxVertices.size());

    for (const Vector3 &vertex : boxVertices)
    {
        const Vector3 planeToVertex = vertex - planeCenter;
        const float signedDistance = planeToVertex.dot(planeNormal);
        if (signedDistance > 0.0f)
        {
            continue;
        }

        const float projectedU = planeToVertex.dot(planeAxisU);
        const float projectedV = planeToVertex.dot(planeAxisV);
        if (std::abs(projectedU) > planeHalfSize.x || std::abs(projectedV) > planeHalfSize.y)
        {
            continue;
        }

        const float depth = -signedDistance;
        const Vector3 planePoint = vertex - planeNormal * (vertex - planeCenter).dot(planeNormal);
        CollisionPoints result;
        result.a = vertex;
        result.b = planePoint;
        result.normal = planeNormal * -1.0f;
        result.depth = depth;
        result.hasCollision = true;
        contacts.push_back(result);
    }

    std::sort(contacts.begin(), contacts.end(), [](const CollisionPoints &a, const CollisionPoints &b) {
        return a.depth > b.depth;
    });
    if (contacts.size() > 4)
    {
        contacts.resize(4);
    }

    return contacts;
}

CollisionPoints CollisionTests::invert(const CollisionPoints &collision)
{
    if (!collision.hasCollision)
    {
        return collision;
    }

    CollisionPoints inverted = collision;
    inverted.a = collision.b;
    inverted.b = collision.a;
    inverted.normal *= -1.0f;
    return inverted;
}
