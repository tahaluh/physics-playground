#include "engine/physics/2d/PhysicsBody2D.h"

#include <cmath>

#include "engine/physics/2d/BorderBoxBody2D.h"
#include "engine/physics/2d/BorderCircleBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include "engine/physics/2d/shapes/RectShape.h"

namespace
{
constexpr float kPositionCorrectionSlop2D = 0.5f;
constexpr float kPositionCorrectionPercent2D = 0.8f;

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

    Vector2 contactOffset = Vector2::zero();
    if (circle)
    {
        const Vector2 radialDirection = (position - borderCircle->getCenter()).lengthSquared() > 0.0f
                                            ? (position - borderCircle->getCenter()).normalized()
                                            : normal;
        const float resolvedRadius = borderCircle->getRadius() - circle->getRadius();
        position = borderCircle->getCenter() + radialDirection * resolvedRadius;
        contactOffset = normal * circle->getRadius();
    }
    else
    {
        const float correctedPenetration = std::max(0.0f, contact.penetration - kPositionCorrectionSlop2D) * kPositionCorrectionPercent2D;
        position -= normal * correctedPenetration;
        contactOffset = contact.contactPoint - getCenterOfMassPosition();
    }

    const float outwardSpeed = velocity.dot(normal);
    if (outwardSpeed > 0.0f)
    {
        velocity -= normal * ((1.0f + surfaceMaterial.restitution) * outwardSpeed);
    }
    applySurfaceFrictionAlongNormal(normal, contactOffset);

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
    if (surfaceMaterial.staticFriction <= 0.0f && surfaceMaterial.dynamicFriction <= 0.0f)
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
    float frictionLimit = Vector2::clamp(surfaceMaterial.dynamicFriction, 0.0f, 1.0f) * referenceSpeed;
    if (targetImpulseMagnitude <= Vector2::clamp(surfaceMaterial.staticFriction, 0.0f, 1.0f) * referenceSpeed)
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

    const float innerRadius = borderCircle->getRadius() - circle->getRadius();
    Vector2 normal = contact.normal;
    if (normal.lengthSquared() <= 0.0f)
        return false;
    position = borderCircle->getCenter() + normal * innerRadius;

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
