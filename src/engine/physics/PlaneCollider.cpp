#include "engine/physics/PlaneCollider.h"

#include "engine/physics/BoxCollider.h"
#include "engine/physics/CollisionTests.h"

namespace
{
constexpr float kPlaneThicknessEpsilon = 0.01f;

Vector3 componentMultiply(const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

Vector3 getPlaneCenter(const PlaneCollider &collider, const Transform &transform)
{
    return transform.position + transform.rotation.toMatrix().transformVector(componentMultiply(collider.offset, transform.scale));
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
    center = getPlaneCenter(collider, transform);
    axisU = rotationMatrix.transformVector(Vector3::right()).normalized();
    axisV = rotationMatrix.transformVector(Vector3::forward()).normalized();
    normal = rotationMatrix.transformVector(Vector3::up()).normalized();
    scaledHalfSize = Vector2(
        std::abs(collider.halfSize.x * transform.scale.x),
        std::abs(collider.halfSize.y * transform.scale.z));
}

}

CollisionPoints PlaneCollider::testCollision(
    const Collider &other,
    const Transform &thisTransform,
    const Transform &otherTransform) const
{
    if (const auto *otherBox = dynamic_cast<const BoxCollider *>(&other))
    {
        return CollisionTests::invert(CollisionTests::testBoxPlane(*otherBox, otherTransform, *this, thisTransform));
    }

    return {};
}

Collider::BroadPhaseBounds PlaneCollider::getBroadPhaseBounds(const Transform &transform) const
{
    Vector3 center;
    Vector3 axisU;
    Vector3 axisV;
    Vector3 normal;
    Vector2 scaledHalfSize;
    getPlaneBasis(*this, transform, center, axisU, axisV, normal, scaledHalfSize);

    const Vector3 extentU = axisU * scaledHalfSize.x;
    const Vector3 extentV = axisV * scaledHalfSize.y;
    const Vector3 extentN = normal * kPlaneThicknessEpsilon;

    std::array<Vector3, 8> corners = {
        center + extentU + extentV + extentN,
        center + extentU + extentV - extentN,
        center + extentU - extentV + extentN,
        center + extentU - extentV - extentN,
        center - extentU + extentV + extentN,
        center - extentU + extentV - extentN,
        center - extentU - extentV + extentN,
        center - extentU - extentV - extentN};

    BroadPhaseBounds bounds;
    bounds.min = corners[0];
    bounds.max = corners[0];
    bounds.valid = true;
    for (const Vector3 &corner : corners)
    {
        bounds.min.x = std::min(bounds.min.x, corner.x);
        bounds.min.y = std::min(bounds.min.y, corner.y);
        bounds.min.z = std::min(bounds.min.z, corner.z);
        bounds.max.x = std::max(bounds.max.x, corner.x);
        bounds.max.y = std::max(bounds.max.y, corner.y);
        bounds.max.z = std::max(bounds.max.z, corner.z);
    }
    return bounds;
}

std::shared_ptr<Collider> PlaneCollider::clone() const
{
    return std::make_shared<PlaneCollider>(*this);
}
