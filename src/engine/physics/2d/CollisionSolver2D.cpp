#include "engine/physics/2d/CollisionSolver2D.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "engine/physics/2d/BorderBoxBody2D.h"
#include "engine/physics/2d/BorderCircleBody2D.h"
#include "engine/physics/2d/Contact2D.h"
#include "engine/physics/2d/Manifold2D.h"
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include "engine/physics/2d/shapes/RectShape.h"
#include "engine/physics/2d/shapes/Shape.h"
#include "engine/scene/2d/Scene2D.h"

namespace
{
// Keep these local and easy to tweak. Lower correction percent helps reduce jitter.
constexpr float kPositionCorrectionSlop2D = 0.01f;
constexpr float kPositionCorrectionPercent2D = 0.2f;
constexpr float kDynamicContactCorrectionPercent2D = 0.5f;
constexpr float kRestitutionThreshold2D = 45.0f;

enum class CollisionPairKind2D
{
    Unsupported,
    CircleCircle,
    CircleRect
};

Vector2 getBodyCenter(PhysicsBody2D &body)
{
    // Rect bodies use top-left as their body position in this engine.
    // For torque/contact math we consistently use the geometric center.
    if (auto *rect = dynamic_cast<RectShape *>(body.getShape()))
    {
        return body.getPosition() + Vector2(rect->getWidth() * 0.5f, rect->getHeight() * 0.5f);
    }
    return body.getPosition();
}

std::array<Vector2, 4> getRotatedRectCorners(const RectShape &rect, const Vector2 &position, const Vector2 &rotation)
{
    const Vector2 center = position + Vector2(rect.getWidth() * 0.5f, rect.getHeight() * 0.5f);
    const Vector2 axisX = rotation.lengthSquared() > 0.0f ? rotation.normalized() : Vector2(1.0f, 0.0f);
    const Vector2 axisY = Vector2::perpendicularLeft(axisX);
    const Vector2 halfExtents(rect.getWidth() * 0.5f, rect.getHeight() * 0.5f);

    return {
        center - axisX * halfExtents.x - axisY * halfExtents.y,
        center + axisX * halfExtents.x - axisY * halfExtents.y,
        center + axisX * halfExtents.x + axisY * halfExtents.y,
        center - axisX * halfExtents.x + axisY * halfExtents.y};
}

struct BorderCircleRectContact2D
{
    bool intersects = false;
    Vector2 normal = Vector2::zero();
    Vector2 contactPoint = Vector2::zero();
    float penetration = 0.0f;
};

BorderCircleRectContact2D buildBorderCircleRectContact(
    const BorderCircleBody2D &borderCircle,
    const RectShape &rect,
    const Vector2 &rectPosition,
    const Vector2 &rectRotation)
{
    BorderCircleRectContact2D result;

    const Vector2 circleCenter = borderCircle.getCenter();
    const Vector2 rectCenter = rectPosition + Vector2(rect.getWidth() * 0.5f, rect.getHeight() * 0.5f);
    Vector2 radialDirection = rectCenter - circleCenter;
    if (radialDirection.lengthSquared() <= 0.0001f)
    {
        radialDirection = Vector2(0.0f, -1.0f);
    }
    radialDirection = radialDirection.normalized();

    const Vector2 axisX = rectRotation.lengthSquared() > 0.0f ? rectRotation.normalized() : Vector2(1.0f, 0.0f);
    const Vector2 axisY = Vector2::perpendicularLeft(axisX);
    const Vector2 halfExtents(rect.getWidth() * 0.5f, rect.getHeight() * 0.5f);

    const float innerRadius = borderCircle.getInnerRadius();
    const float outerRadius = borderCircle.getOuterRadius();

    const float centerDistance = (rectCenter - circleCenter).length();
    const float supportRadius =
        std::abs(radialDirection.dot(axisX)) * halfExtents.x +
        std::abs(radialDirection.dot(axisY)) * halfExtents.y;
    const float minRadialDistance = std::max(0.0f, centerDistance - supportRadius);
    const float maxRadialDistance = centerDistance + supportRadius;

    // The ring collides only with the solid annulus between inner and outer.
    if (maxRadialDistance <= innerRadius || minRadialDistance >= outerRadius)
    {
        return result;
    }

    const float innerPenetration = std::max(0.0f, maxRadialDistance - innerRadius);
    const float outerPenetration = std::max(0.0f, outerRadius - minRadialDistance);
    const bool preferInnerFace = centerDistance <= innerRadius;
    const bool preferOuterFace = centerDistance >= outerRadius;

    if (innerPenetration > 0.0f && (outerPenetration <= 0.0f || preferInnerFace || (!preferOuterFace && innerPenetration <= outerPenetration)))
    {
        result.intersects = true;
        result.normal = radialDirection;
        result.penetration = innerPenetration;
        result.contactPoint = circleCenter + radialDirection * innerRadius;
        return result;
    }

    result.intersects = true;
    result.normal = radialDirection * -1.0f;
    result.penetration = outerPenetration;
    result.contactPoint = circleCenter + radialDirection * outerRadius;
    return result;
}

Vector2 getContactVelocity(PhysicsBody2D &body, const Vector2 &contactPoint)
{
    const Vector2 offset = contactPoint - getBodyCenter(body);
    return body.getVelocity() + Vector2::perpendicularLeft(offset) * body.getAngularVelocity();
}

bool isCircleCirclePair(const Shape *shapeA, const Shape *shapeB)
{
    return dynamic_cast<const CircleShape *>(shapeA) && dynamic_cast<const CircleShape *>(shapeB);
}

bool isCircleRectPair(const Shape *shapeA, const Shape *shapeB)
{
    return (dynamic_cast<const CircleShape *>(shapeA) && dynamic_cast<const RectShape *>(shapeB)) ||
           (dynamic_cast<const RectShape *>(shapeA) && dynamic_cast<const CircleShape *>(shapeB));
}

CollisionPairKind2D getPairKind(const Shape *shapeA, const Shape *shapeB)
{
    if (isCircleCirclePair(shapeA, shapeB))
    {
        return CollisionPairKind2D::CircleCircle;
    }
    if (isCircleRectPair(shapeA, shapeB))
    {
        return CollisionPairKind2D::CircleRect;
    }
    return CollisionPairKind2D::Unsupported;
}

bool needsExplicitBorderCircleRectBroadphase(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB)
{
    return (dynamic_cast<BorderCircleBody2D *>(&bodyA) && dynamic_cast<RectShape *>(bodyB.getShape())) ||
           (dynamic_cast<RectShape *>(bodyA.getShape()) && dynamic_cast<BorderCircleBody2D *>(&bodyB));
}

void applyPositionCorrection(
    PhysicsBody2D &bodyA,
    PhysicsBody2D &bodyB,
    const Vector2 &normal,
    float penetration,
    float correctionPercent)
{
    if (normal.lengthSquared() <= 0.0f || penetration <= 0.0f)
    {
        return;
    }

    const float inverseMassSum = bodyA.getInverseMass() + bodyB.getInverseMass();
    if (inverseMassSum <= 0.0f)
    {
        return;
    }

    const float correctedPenetration =
        std::max(0.0f, penetration - kPositionCorrectionSlop2D) * correctionPercent;
    if (correctedPenetration <= 0.0f)
    {
        return;
    }

    bodyA.setPosition(bodyA.getPosition() - normal * (correctedPenetration * (bodyA.getInverseMass() / inverseMassSum)));
    bodyB.setPosition(bodyB.getPosition() + normal * (correctedPenetration * (bodyB.getInverseMass() / inverseMassSum)));
}

void applyPositionCorrection(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, const Vector2 &normal, float penetration)
{
    applyPositionCorrection(bodyA, bodyB, normal, penetration, kPositionCorrectionPercent2D);
}

void applyDynamicFrictionImpulse(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, const Contact2D &contactA, float normalImpulseMagnitude)
{
    const Vector2 contactVelocityA = getContactVelocity(bodyA, contactA.contactPoint);
    const Vector2 contactVelocityB = getContactVelocity(bodyB, contactA.contactPoint);
    const Vector2 relativeVelocity = contactVelocityB - contactVelocityA;
    const float normalSpeed = relativeVelocity.dot(contactA.normal);
    const Vector2 tangentialVelocity = relativeVelocity - contactA.normal * normalSpeed;
    const float tangentialSpeed = tangentialVelocity.length();
    if (tangentialSpeed <= 0.0f)
    {
        return;
    }

    const float staticFriction =
        (bodyA.getSurfaceMaterial().staticFriction + bodyB.getSurfaceMaterial().staticFriction) * 0.5f;
    const float dynamicFriction =
        (bodyA.getSurfaceMaterial().dynamicFriction + bodyB.getSurfaceMaterial().dynamicFriction) * 0.5f;

    const Vector2 tangent = tangentialVelocity / tangentialSpeed;
    const Vector2 offsetA = contactA.contactPoint - getBodyCenter(bodyA);
    const Vector2 offsetB = contactA.contactPoint - getBodyCenter(bodyB);
    const float radiusCrossTangentA = Vector2::cross(offsetA, tangent);
    const float radiusCrossTangentB = Vector2::cross(offsetB, tangent);
    const float denominator =
        bodyA.getInverseMass() +
        bodyB.getInverseMass() +
        radiusCrossTangentA * radiusCrossTangentA * bodyA.getInverseMomentOfInertia() +
        radiusCrossTangentB * radiusCrossTangentB * bodyB.getInverseMomentOfInertia();
    if (denominator <= 0.0f)
    {
        return;
    }

    const float desiredImpulseMagnitude = tangentialSpeed / denominator;
    const float staticLimit = staticFriction * normalImpulseMagnitude;
    const float dynamicLimit = dynamicFriction * normalImpulseMagnitude;

    float signedImpulseMagnitude = 0.0f;
    if (desiredImpulseMagnitude <= staticLimit)
    {
        // Static friction can fully cancel the tangential motion at the contact.
        signedImpulseMagnitude = -desiredImpulseMagnitude;
    }
    else
    {
        // Dynamic friction opposes sliding but is limited by the kinetic coefficient.
        signedImpulseMagnitude = -dynamicLimit;
    }

    const Vector2 impulse = tangent * signedImpulseMagnitude;
    bodyA.setVelocity(bodyA.getVelocity() - impulse * bodyA.getInverseMass());
    bodyB.setVelocity(bodyB.getVelocity() + impulse * bodyB.getInverseMass());
    bodyA.setAngularVelocity(bodyA.getAngularVelocity() - Vector2::cross(offsetA, impulse) * bodyA.getInverseMomentOfInertia());
    bodyB.setAngularVelocity(bodyB.getAngularVelocity() + Vector2::cross(offsetB, impulse) * bodyB.getInverseMomentOfInertia());
}

float computeEffectiveRestitution2D(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, float impactSpeed)
{
    float restitution = (bodyA.getSurfaceMaterial().restitution + bodyB.getSurfaceMaterial().restitution) * 0.5f;
    if (impactSpeed < kRestitutionThreshold2D)
    {
        restitution = 0.0f;
    }
    return restitution;
}

float computeCombinedFrictionStrength2D(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB)
{
    return std::max(
        0.0f,
        (bodyA.getSurfaceMaterial().staticFriction +
         bodyA.getSurfaceMaterial().dynamicFriction +
         bodyB.getSurfaceMaterial().staticFriction +
         bodyB.getSurfaceMaterial().dynamicFriction) *
            0.25f);
}

bool resolveDynamicContactImpulse2D(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, const Contact2D &contactA, float *outNormalImpulseMagnitude = nullptr)
{
    if (bodyA.isStatic() || bodyB.isStatic() || contactA.penetration <= 0.0f)
    {
        return false;
    }

    const Vector2 normal = contactA.normal;
    if (normal.lengthSquared() <= 0.0f)
    {
        return false;
    }

    applyPositionCorrection(bodyA, bodyB, normal, contactA.penetration, kDynamicContactCorrectionPercent2D);

    const Vector2 contactPoint = contactA.contactPoint;
    const Vector2 offsetA = contactPoint - getBodyCenter(bodyA);
    const Vector2 offsetB = contactPoint - getBodyCenter(bodyB);
    const Vector2 contactVelocityA = getContactVelocity(bodyA, contactPoint);
    const Vector2 contactVelocityB = getContactVelocity(bodyB, contactPoint);
    const Vector2 relativeVelocity = contactVelocityB - contactVelocityA;
    const float normalSpeed = relativeVelocity.dot(normal);
    if (normalSpeed > 0.0f)
    {
        return true;
    }

    const float restitution = computeEffectiveRestitution2D(bodyA, bodyB, -normalSpeed);
    const float radiusCrossNormalA = Vector2::cross(offsetA, normal);
    const float radiusCrossNormalB = Vector2::cross(offsetB, normal);
    const float denominator =
        bodyA.getInverseMass() +
        bodyB.getInverseMass() +
        radiusCrossNormalA * radiusCrossNormalA * bodyA.getInverseMomentOfInertia() +
        radiusCrossNormalB * radiusCrossNormalB * bodyB.getInverseMomentOfInertia();
    if (denominator <= 0.0f)
    {
        return true;
    }

    const float normalImpulseMagnitude = -(1.0f + restitution) * normalSpeed / denominator;
    const Vector2 impulse = normal * normalImpulseMagnitude;
    bodyA.setVelocity(bodyA.getVelocity() - impulse * bodyA.getInverseMass());
    bodyB.setVelocity(bodyB.getVelocity() + impulse * bodyB.getInverseMass());
    bodyA.setAngularVelocity(bodyA.getAngularVelocity() - Vector2::cross(offsetA, impulse) * bodyA.getInverseMomentOfInertia());
    bodyB.setAngularVelocity(bodyB.getAngularVelocity() + Vector2::cross(offsetB, impulse) * bodyB.getInverseMomentOfInertia());

    applyDynamicFrictionImpulse(bodyA, bodyB, contactA, normalImpulseMagnitude);

    if (outNormalImpulseMagnitude)
    {
        *outNormalImpulseMagnitude = normalImpulseMagnitude;
    }
    return true;
}

bool resolveDynamicCircleCircleCollision(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, const Contact2D &contactA)
{
    auto *circleA = dynamic_cast<CircleShape *>(bodyA.getShape());
    auto *circleB = dynamic_cast<CircleShape *>(bodyB.getShape());
    if (!circleA || !circleB)
    {
        return false;
    }
    if (!resolveDynamicContactImpulse2D(bodyA, bodyB, contactA))
    {
        return false;
    }

    return true;
}

bool resolveDynamicCircleRectCollision(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, const Contact2D &contactA)
{
    auto *circleA = dynamic_cast<CircleShape *>(bodyA.getShape());
    auto *rectB = dynamic_cast<RectShape *>(bodyB.getShape());
    if (!circleA || !rectB)
    {
        return false;
    }
    if (!resolveDynamicContactImpulse2D(bodyA, bodyB, contactA))
    {
        return false;
    }

    return true;
}

bool resolvePair(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, const Contact2D &contactA, const Contact2D &contactB)
{
    if (dynamic_cast<BorderCircleBody2D *>(&bodyA) || dynamic_cast<BorderCircleBody2D *>(&bodyB))
    {
        return false;
    }

    switch (getPairKind(bodyA.getShape(), bodyB.getShape()))
    {
    case CollisionPairKind2D::CircleCircle:
        return resolveDynamicCircleCircleCollision(bodyA, bodyB, contactA);

    case CollisionPairKind2D::CircleRect:
        // Normalize this resolver so it always receives circle as A and rect as B.
        if (dynamic_cast<CircleShape *>(bodyA.getShape()) && dynamic_cast<RectShape *>(bodyB.getShape()))
        {
            return resolveDynamicCircleRectCollision(bodyA, bodyB, contactA);
        }
        return resolveDynamicCircleRectCollision(bodyB, bodyA, contactB);

    case CollisionPairKind2D::Unsupported:
    default:
        return false;
    }
}
} // namespace

void CollisionSolver2D::solve(Scene2D &scene, std::vector<Manifold2D> &manifolds) const
{
    manifolds.clear();
    auto &bodies = scene.getBodies();

    for (size_t i = 0; i < bodies.size(); ++i)
    {
        for (size_t j = i + 1; j < bodies.size(); ++j)
        {
            PhysicsBody2D &bodyA = *bodies[i];
            PhysicsBody2D &bodyB = *bodies[j];
            Shape *shapeA = bodyA.getShape();
            Shape *shapeB = bodyB.getShape();
            if (!shapeA || !shapeB || !bodyA.isCollisionEnabled() || !bodyB.isCollisionEnabled())
            {
                continue;
            }

            // The generic broadphase for RectShape is axis-aligned. Skip it for
            // border-circle vs rect so the manifold can use the rotation-aware path.
            if (!needsExplicitBorderCircleRectBroadphase(bodyA, bodyB) &&
                !shapeA->collidesWith(*shapeB, bodyA.getPosition(), bodyB.getPosition()))
            {
                continue;
            }

            Manifold2D manifold;
            if (!buildManifold(bodyA, bodyB, manifold))
            {
                continue;
            }

            manifolds.push_back(manifold);

            Contact2D contactA;
            Contact2D contactB;
            buildContactsFromManifold(manifolds.back(), contactA, contactB);

            // Resolve first, but always fire callbacks afterwards.
            resolvePair(bodyA, bodyB, contactA, contactB);

            bodyA.onCollision(contactA);
            bodyB.onCollision(contactB);
        }
    }
}

bool CollisionSolver2D::buildManifold(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, Manifold2D &manifold) const
{
    auto *borderCircleA = dynamic_cast<BorderCircleBody2D *>(&bodyA);
    auto *borderCircleB = dynamic_cast<BorderCircleBody2D *>(&bodyB);
    auto *circleA = dynamic_cast<CircleShape *>(bodyA.getShape());
    auto *circleB = dynamic_cast<CircleShape *>(bodyB.getShape());

    if (borderCircleA && circleB)
    {
        const Vector2 delta = bodyB.getPosition() - borderCircleA->getCenter();
        const float distanceSquared = delta.lengthSquared();
        const float innerRadius = std::max(0.0f, borderCircleA->getInnerRadius() - circleB->getRadius());
        const float outerRadius = borderCircleA->getOuterRadius() + circleB->getRadius();
        const float innerRadiusSquared = innerRadius * innerRadius;
        const float outerRadiusSquared = outerRadius * outerRadius;
        if (distanceSquared < innerRadiusSquared || distanceSquared > outerRadiusSquared)
        {
            return false;
        }

        const float distance = std::sqrt(std::max(distanceSquared, 0.0001f));
        const Vector2 normal = delta / distance;
        const float innerPenetration = std::max(0.0f, distance - innerRadius);
        const float outerPenetration = std::max(0.0f, outerRadius - distance);
        const bool preferInnerFace = distance <= innerRadius;
        const bool preferOuterFace = distance >= outerRadius;

        manifold.bodyA = &bodyA;
        manifold.bodyB = &bodyB;
        if (innerPenetration > 0.0f && (outerPenetration <= 0.0f || preferInnerFace || (!preferOuterFace && innerPenetration <= outerPenetration)))
        {
            manifold.normal = normal * -1.0f;
            manifold.penetration = innerPenetration;
            manifold.contactPoints[0] = borderCircleA->getCenter() + normal * borderCircleA->getInnerRadius();
        }
        else
        {
            manifold.normal = normal;
            manifold.penetration = outerPenetration;
            manifold.contactPoints[0] = borderCircleA->getCenter() + normal * borderCircleA->getOuterRadius();
        }
        manifold.contactCount = 1;
        return true;
    }

    if (circleA && circleB)
    {
        const Vector2 delta = bodyB.getPosition() - bodyA.getPosition();
        const float distanceSquared = delta.lengthSquared();
        const float radiusSum = circleA->getRadius() + circleB->getRadius();
        if (distanceSquared > radiusSum * radiusSum)
        {
            return false;
        }

        Vector2 normal(1.0f, 0.0f);
        float distance = 0.0f;
        if (distanceSquared > 0.0001f)
        {
            distance = std::sqrt(distanceSquared);
            normal = delta / distance;
        }

        manifold.bodyA = &bodyA;
        manifold.bodyB = &bodyB;
        manifold.normal = normal;
        manifold.penetration = radiusSum - distance;
        manifold.contactPoints[0] = bodyA.getPosition() + normal * circleA->getRadius();
        manifold.contactCount = 1;
        return true;
    }

    if (circleA && borderCircleB)
    {
        const Vector2 delta = bodyA.getPosition() - borderCircleB->getCenter();
        const float distanceSquared = delta.lengthSquared();
        const float innerRadius = std::max(0.0f, borderCircleB->getInnerRadius() - circleA->getRadius());
        const float outerRadius = borderCircleB->getOuterRadius() + circleA->getRadius();
        const float innerRadiusSquared = innerRadius * innerRadius;
        const float outerRadiusSquared = outerRadius * outerRadius;
        if (distanceSquared < innerRadiusSquared || distanceSquared > outerRadiusSquared)
        {
            return false;
        }

        const float distance = std::sqrt(std::max(distanceSquared, 0.0001f));
        const Vector2 normal = delta / distance;
        const float innerPenetration = std::max(0.0f, distance - innerRadius);
        const float outerPenetration = std::max(0.0f, outerRadius - distance);
        const bool preferInnerFace = distance <= innerRadius;
        const bool preferOuterFace = distance >= outerRadius;
        manifold.bodyA = &bodyA;
        manifold.bodyB = &bodyB;
        if (innerPenetration > 0.0f && (outerPenetration <= 0.0f || preferInnerFace || (!preferOuterFace && innerPenetration <= outerPenetration)))
        {
            manifold.normal = normal;
            manifold.penetration = innerPenetration;
            manifold.contactPoints[0] = borderCircleB->getCenter() + normal * borderCircleB->getInnerRadius();
        }
        else
        {
            manifold.normal = normal * -1.0f;
            manifold.penetration = outerPenetration;
            manifold.contactPoints[0] = borderCircleB->getCenter() + normal * borderCircleB->getOuterRadius();
        }
        manifold.contactCount = 1;
        return true;
    }

    auto *rectA = dynamic_cast<RectShape *>(bodyA.getShape());
    auto *rectB = dynamic_cast<RectShape *>(bodyB.getShape());
    auto buildCircleRectManifold =
        [](PhysicsBody2D &circleBody, CircleShape &circle, PhysicsBody2D &rectBody, RectShape &rect, Manifold2D &manifold, bool circleIsBodyA) -> bool
    {
        // Rect bodies keep top-left as position, but the rendered square rotates
        // around its center. Build the manifold against that same oriented box.
        const Vector2 &circlePosition = circleBody.getPosition();
        const Vector2 rectCenter = getBodyCenter(rectBody);
        const Vector2 axisX = rectBody.getRotation().lengthSquared() > 0.0f
                                  ? rectBody.getRotation().normalized()
                                  : Vector2(1.0f, 0.0f);
        const Vector2 axisY = Vector2::perpendicularLeft(axisX);
        const Vector2 halfExtents(rect.getWidth() * 0.5f, rect.getHeight() * 0.5f);

        const Vector2 circleToRect = circlePosition - rectCenter;
        const Vector2 localCircle(
            circleToRect.dot(axisX),
            circleToRect.dot(axisY));

        const Vector2 clampedLocal(
            Vector2::clamp(localCircle.x, -halfExtents.x, halfExtents.x),
            Vector2::clamp(localCircle.y, -halfExtents.y, halfExtents.y));
        const Vector2 closestPoint = rectCenter + axisX * clampedLocal.x + axisY * clampedLocal.y;
        const Vector2 deltaToCircle = circlePosition - closestPoint;
        const float distanceSquared = deltaToCircle.lengthSquared();
        const float radius = circle.getRadius();
        if (distanceSquared > radius * radius)
        {
            return false;
        }

        Vector2 normal = Vector2::zero();
        Vector2 contactPoint = closestPoint;
        float penetration = 0.0f;

        if (distanceSquared > 0.0001f)
        {
            const float distance = std::sqrt(distanceSquared);
            const Vector2 outwardFromRect = deltaToCircle / distance;
            normal = circleIsBodyA ? (outwardFromRect * -1.0f) : outwardFromRect;
            penetration = radius - distance;
        }
        else
        {
            // Circle center is inside the OBB. Pick the nearest face in local space
            // so the fallback normal stays aligned with the rendered square.
            const float dx = halfExtents.x - std::abs(localCircle.x);
            const float dy = halfExtents.y - std::abs(localCircle.y);

            Vector2 outwardFromRect = Vector2::zero();
            if (dx <= dy)
            {
                outwardFromRect = axisX * (localCircle.x >= 0.0f ? 1.0f : -1.0f);
                contactPoint = rectCenter + axisX * (halfExtents.x * (localCircle.x >= 0.0f ? 1.0f : -1.0f)) +
                               axisY * localCircle.y;
                penetration = std::max(0.0f, radius - dx);
            }
            else
            {
                outwardFromRect = axisY * (localCircle.y >= 0.0f ? 1.0f : -1.0f);
                contactPoint = rectCenter + axisX * localCircle.x +
                               axisY * (halfExtents.y * (localCircle.y >= 0.0f ? 1.0f : -1.0f));
                penetration = std::max(0.0f, radius - dy);
            }

            normal = circleIsBodyA ? (outwardFromRect * -1.0f) : outwardFromRect;
        }

        manifold.bodyA = circleIsBodyA ? &circleBody : &rectBody;
        manifold.bodyB = circleIsBodyA ? &rectBody : &circleBody;
        manifold.normal = normal;
        manifold.penetration = penetration;
        manifold.contactPoints[0] = contactPoint;
        manifold.contactCount = 1;
        return true;
    };

    if (borderCircleA && rectB)
    {
        const BorderCircleRectContact2D contact =
            buildBorderCircleRectContact(*borderCircleA, *rectB, bodyB.getPosition(), bodyB.getRotation());
        if (!contact.intersects)
        {
            return false;
        }

        manifold.bodyA = &bodyA;
        manifold.bodyB = &bodyB;
        manifold.normal = contact.normal * -1.0f;
        manifold.penetration = contact.penetration;
        manifold.contactPoints[0] = contact.contactPoint;
        manifold.contactCount = 1;
        return true;
    }

    if (rectA && borderCircleB)
    {
        const BorderCircleRectContact2D contact =
            buildBorderCircleRectContact(*borderCircleB, *rectA, bodyA.getPosition(), bodyA.getRotation());
        if (!contact.intersects)
        {
            return false;
        }

        manifold.bodyA = &bodyA;
        manifold.bodyB = &bodyB;
        manifold.normal = contact.normal;
        manifold.penetration = contact.penetration;
        manifold.contactPoints[0] = contact.contactPoint;
        manifold.contactCount = 1;
        return true;
    }

    if (circleA && rectB)
    {
        return buildCircleRectManifold(bodyA, *circleA, bodyB, *rectB, manifold, true);
    }

    if (rectA && circleB)
    {
        return buildCircleRectManifold(bodyB, *circleB, bodyA, *rectA, manifold, false);
    }

    return false;
}

void CollisionSolver2D::buildContactsFromManifold(const Manifold2D &manifold, Contact2D &contactA, Contact2D &contactB) const
{
    const Vector2 contactPoint = manifold.contactCount > 0 ? manifold.contactPoints[0] : Vector2::zero();
    contactA = Contact2D{manifold.bodyA, manifold.bodyB, manifold.normal, manifold.penetration, contactPoint};
    contactB = Contact2D{manifold.bodyB, manifold.bodyA, manifold.normal * -1.0f, manifold.penetration, contactPoint};
}

bool CollisionSolver2D::resolveDynamicCircleCollision(const Contact2D &contactA, const Contact2D &contactB) const
{
    if (!contactA.self || !contactB.self)
    {
        return false;
    }
    return resolveDynamicCircleCircleCollision(*contactA.self, *contactB.self, contactA);
}
