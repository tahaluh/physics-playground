#include "engine/physics/2d/PhysicsBody2D.h"

#include <cmath>

#include "engine/physics/2d/BorderBoxBody2D.h"
#include "engine/physics/2d/BorderCircleBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include "engine/physics/2d/shapes/RectShape.h"

namespace
{
constexpr float kPositionCorrectionSlop2D = 0.01f;
constexpr float kPositionCorrectionPercent2D = 0.2f;
constexpr float kBorderRestitutionThreshold2D = 55.0f;

CircleShape *getCircleShape(PhysicsBody2D &body)
{
    return dynamic_cast<CircleShape *>(body.getShape());
}

Vector2 getFarthestRectCorner(const RectShape &rect, const Vector2 &position, const Vector2 &referencePoint)
{
    const Vector2 corners[] = {
        position,
        position + Vector2(rect.getWidth(), 0.0f),
        position + Vector2(rect.getWidth(), rect.getHeight()),
        position + Vector2(0.0f, rect.getHeight())};

    Vector2 farthestCorner = corners[0];
    float farthestDistanceSquared = (corners[0] - referencePoint).lengthSquared();
    for (int i = 1; i < 4; ++i)
    {
        const float distanceSquared = (corners[i] - referencePoint).lengthSquared();
        if (distanceSquared > farthestDistanceSquared)
        {
            farthestDistanceSquared = distanceSquared;
            farthestCorner = corners[i];
        }
    }
    return farthestCorner;
}
}

bool PhysicsBody2D::resolveBorderCircleCollision(const Contact2D &contact, float stopThreshold)
{
    auto *borderCircle = dynamic_cast<BorderCircleBody2D *>(contact.other);
    auto *circle = getCircleShape(*this);
    auto *rect = dynamic_cast<RectShape *>(getShape());
    if (!borderCircle || (!circle && !rect))
        return false;

    Vector2 normal = contact.normal;
    if (normal.lengthSquared() <= 0.0f)
        return false;

    const float combinedRestitution =
        std::min(surfaceMaterial.restitution, borderCircle->getSurfaceMaterial().restitution);
    // Use the geometric mean so a smooth wall stays smooth even if the moving
    // body has high friction on its own.
    const float combinedStaticFriction =
        std::sqrt(std::max(0.0f, surfaceMaterial.staticFriction) *
                  std::max(0.0f, borderCircle->getSurfaceMaterial().staticFriction));
    const float combinedDynamicFriction =
        std::sqrt(std::max(0.0f, surfaceMaterial.dynamicFriction) *
                  std::max(0.0f, borderCircle->getSurfaceMaterial().dynamicFriction));

    Vector2 contactOffset = Vector2::zero();
    if (circle)
    {
        const Vector2 radialDirection = (position - borderCircle->getCenter()).lengthSquared() > 0.0f
                                            ? (position - borderCircle->getCenter()).normalized()
                                            : normal;
        const bool collidedWithInnerFace = normal.dot(radialDirection) >= 0.0f;
        const float resolvedRadius = collidedWithInnerFace
                                         ? std::max(0.0f, borderCircle->getInnerRadius() - circle->getRadius())
                                         : borderCircle->getOuterRadius() + circle->getRadius();
        position = borderCircle->getCenter() + radialDirection * resolvedRadius;
        contactOffset = normal * circle->getRadius();
    }
    else
    {
        const Vector2 centerOfMass = getCenterOfMassPosition();
        Vector2 radialDirection = centerOfMass - borderCircle->getCenter();
        if (radialDirection.lengthSquared() <= 0.0f)
        {
            radialDirection = Vector2(0.0f, -1.0f);
        }
        radialDirection = radialDirection.normalized();

        const Vector2 axisX = rotation.lengthSquared() > 0.0f ? rotation.normalized() : Vector2(1.0f, 0.0f);
        const Vector2 axisY = Vector2::perpendicularLeft(axisX);
        const Vector2 halfExtents(rect->getWidth() * 0.5f, rect->getHeight() * 0.5f);
        const float supportRadius =
            std::abs(radialDirection.dot(axisX)) * halfExtents.x +
            std::abs(radialDirection.dot(axisY)) * halfExtents.y;
        const float centerDistance = (centerOfMass - borderCircle->getCenter()).length();

        // Defensive guard: if the box is fully inside the inner hole or fully
        // outside the outer circle, ignore any stale/incorrect border contact.
        if (centerDistance + supportRadius <= borderCircle->getInnerRadius() ||
            centerDistance - supportRadius >= borderCircle->getOuterRadius())
        {
            return true;
        }

        const float correction = std::max(0.0f, contact.penetration - kPositionCorrectionSlop2D) * kPositionCorrectionPercent2D;
        if (correction > 0.0f)
        {
            position -= normal * correction;
        }

        contactOffset = contact.contactPoint - getCenterOfMassPosition();

        // For the box against the circular border, use the actual contact point
        // to generate angular response instead of only correcting position.
        const Vector2 contactVelocity =
            velocity + Vector2::perpendicularLeft(contactOffset) * angularVelocity;
        const float approachSpeed = contactVelocity.dot(normal);
        if (approachSpeed > 0.0f)
        {
            const float radiusCrossNormal = Vector2::cross(contactOffset, normal);
            const float denominator =
                inverseMass + (radiusCrossNormal * radiusCrossNormal) * inverseMomentOfInertia;
            if (denominator > 0.0f)
            {
                const float effectiveRestitution =
                    approachSpeed > kBorderRestitutionThreshold2D ? combinedRestitution : 0.0f;
                const float impulseMagnitude =
                    -((1.0f + effectiveRestitution) * approachSpeed) / denominator;
                const Vector2 impulse = normal * impulseMagnitude;
                velocity += impulse * inverseMass;
                angularVelocity += Vector2::cross(contactOffset, impulse) * inverseMomentOfInertia;
            }
        }
    }

    const float outwardSpeed = velocity.dot(normal);
    if (circle && outwardSpeed > 0.0f)
    {
        const float effectiveRestitution =
            outwardSpeed > kBorderRestitutionThreshold2D ? combinedRestitution : 0.0f;
        velocity -= normal * ((1.0f + effectiveRestitution) * outwardSpeed);
    }
    applySurfaceFrictionAlongNormal(normal, contactOffset, combinedStaticFriction, combinedDynamicFriction);

    if (stopThreshold > 0.0f && velocity.length() < stopThreshold)
    {
        velocity = Vector2::zero();
    }

    return true;
}

bool PhysicsBody2D::resolveBorderBoxCollision(const Contact2D &contact, float stopThreshold)
{
    auto *box = dynamic_cast<BorderBoxBody2D *>(contact.other);
    auto *circle = getCircleShape(*this);
    if (!box || !circle)
        return false;

    const float radius = circle->getRadius();
    Vector2 normal = contact.normal;
    if (normal.lengthSquared() <= 0.0f)
        return false;

    if (normal.x > 0.0f)
        position.x = box->left() + radius;
    else if (normal.x < 0.0f)
        position.x = box->right() - radius;

    if (normal.y > 0.0f)
        position.y = box->top() + radius;
    else if (normal.y < 0.0f)
        position.y = box->bottom() - radius;

    const float outwardSpeed = velocity.dot(normal);
    if (outwardSpeed > 0.0f)
    {
        velocity -= normal * ((1.0f + surfaceMaterial.restitution) * outwardSpeed);
    }
    applySurfaceFrictionAlongNormal(normal, normal * -radius);
    if (stopThreshold > 0.0f && velocity.length() < stopThreshold)
    {
        velocity = Vector2::zero();
    }

    return true;
}

void PhysicsBody2D::applySurfaceFrictionAlongNormal(const Vector2 &normal, const Vector2 &contactOffset)
{
    applySurfaceFrictionAlongNormal(
        normal,
        contactOffset,
        surfaceMaterial.staticFriction,
        surfaceMaterial.dynamicFriction);
}

void PhysicsBody2D::applySurfaceFrictionAlongNormal(
    const Vector2 &normal,
    const Vector2 &contactOffset,
    float staticFriction,
    float dynamicFriction)
{
    if (staticFriction <= 0.0f && dynamicFriction <= 0.0f)
        return;

    const Vector2 contactVelocity =
        velocity + Vector2::perpendicularLeft(contactOffset) * angularVelocity;
    const float normalSpeed = contactVelocity.dot(normal);
    const Vector2 tangentialVelocity = contactVelocity - normal * normalSpeed;
    const float tangentialSpeed = tangentialVelocity.length();
    if (tangentialSpeed <= 0.0f)
    {
        return;
    }

    const Vector2 tangent = tangentialVelocity / tangentialSpeed;
    const float radiusCrossTangent = Vector2::cross(contactOffset, tangent);
    const float denominator =
        inverseMass + (radiusCrossTangent * radiusCrossTangent) * inverseMomentOfInertia;
    if (denominator <= 0.0f)
    {
        return;
    }

    const float targetImpulseMagnitude = tangentialSpeed / denominator;
    const float referenceSpeed = std::max(std::abs(normalSpeed), tangentialSpeed);
    float frictionLimit = Vector2::clamp(dynamicFriction, 0.0f, 1.0f) * referenceSpeed;
    if (targetImpulseMagnitude <= Vector2::clamp(staticFriction, 0.0f, 1.0f) * referenceSpeed)
    {
        frictionLimit = targetImpulseMagnitude;
    }
    const float impulseMagnitude = -std::min(targetImpulseMagnitude, frictionLimit);
    const Vector2 impulse = tangent * impulseMagnitude;
    velocity += impulse * inverseMass;
    angularVelocity += Vector2::cross(contactOffset, impulse) * inverseMomentOfInertia;
}

Vector2 PhysicsBody2D::getCenterOfMassPosition() const
{
    if (const auto *rect = dynamic_cast<const RectShape *>(shape.get()))
    {
        return position + Vector2(rect->getWidth() * 0.5f, rect->getHeight() * 0.5f);
    }
    return position;
}

bool PhysicsBody2D::resolveBorderCircleAxisInvertCollision(const Contact2D &contact)
{
    auto *borderCircle = dynamic_cast<BorderCircleBody2D *>(contact.other);
    auto *circle = getCircleShape(*this);
    if (!borderCircle || !circle)
        return false;

    Vector2 normal = contact.normal;
    if (normal.lengthSquared() <= 0.0f)
        return false;
    const Vector2 radialDirection = (position - borderCircle->getCenter()).lengthSquared() > 0.0f
                                        ? (position - borderCircle->getCenter()).normalized()
                                        : normal;
    const bool collidedWithInnerFace = normal.dot(radialDirection) >= 0.0f;
    const float resolvedRadius = collidedWithInnerFace
                                     ? std::max(0.0f, borderCircle->getInnerRadius() - circle->getRadius())
                                     : borderCircle->getOuterRadius() + circle->getRadius();
    position = borderCircle->getCenter() + radialDirection * resolvedRadius;

    if (std::abs(normal.x) > std::abs(normal.y))
    {
        velocity.x = -velocity.x * surfaceMaterial.restitution;
    }
    else
    {
        velocity.y = -velocity.y * surfaceMaterial.restitution;
    }

    return true;
}

float PhysicsBody2D::computeMomentOfInertia() const
{
    if (isStatic())
    {
        return 0.0f;
    }

    if (const auto *circle = dynamic_cast<const CircleShape *>(shape.get()))
    {
        const float radius = circle->getRadius();
        return 0.5f * mass * radius * radius;
    }

    if (const auto *rect = dynamic_cast<const RectShape *>(shape.get()))
    {
        return (mass * (rect->getWidth() * rect->getWidth() + rect->getHeight() * rect->getHeight())) / 12.0f;
    }

    return 0.0f;
}
