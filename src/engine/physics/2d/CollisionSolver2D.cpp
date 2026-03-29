#include "engine/physics/2d/CollisionSolver2D.h"

#include <algorithm>
#include <vector>
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/BorderBoxBody2D.h"
#include "engine/physics/2d/BorderCircleBody2D.h"
#include "engine/physics/2d/Contact2D.h"
#include "engine/physics/2d/Manifold2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include "engine/physics/2d/shapes/Shape.h"
#include "engine/physics/2d/shapes/RectShape.h"
#include "engine/scene/2d/Scene2D.h"

namespace
{
constexpr float kPositionCorrectionSlop2D = 0.5f;
constexpr float kPositionCorrectionPercent2D = 0.8f;

Vector2 getBodyCenter(PhysicsBody2D &body)
{
    if (auto *rect = dynamic_cast<RectShape *>(body.getShape()))
    {
        return body.getPosition() + Vector2(rect->getWidth() * 0.5f, rect->getHeight() * 0.5f);
    }
    return body.getPosition();
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
    const auto corners = getRotatedRectCorners(rect, rectPosition, rectRotation);
    const Vector2 circleCenter = borderCircle.getCenter();

    Vector2 farthestCorner = corners[0];
    float farthestDistanceSquared = (corners[0] - circleCenter).lengthSquared();
    for (int i = 1; i < 4; ++i)
    {
        const float distanceSquared = (corners[i] - circleCenter).lengthSquared();
        if (distanceSquared > farthestDistanceSquared)
        {
            farthestDistanceSquared = distanceSquared;
            farthestCorner = corners[i];
        }
    }

    const float borderRadius = borderCircle.getRadius();
    if (farthestDistanceSquared <= borderRadius * borderRadius)
    {
        return result;
    }

    const float farthestDistance = std::sqrt(std::max(farthestDistanceSquared, 0.0001f));
    const Vector2 radialNormal =
        farthestDistance > 0.0f ? (farthestCorner - circleCenter) / farthestDistance : Vector2(1.0f, 0.0f);

    result.intersects = true;
    result.normal = radialNormal;
    result.penetration = farthestDistance - borderRadius;
    result.contactPoint = farthestCorner;
    return result;
}

Vector2 getContactVelocity(PhysicsBody2D &body, const Vector2 &contactPoint)
{
    const Vector2 offset = contactPoint - getBodyCenter(body);
    return body.getVelocity() + Vector2::perpendicularLeft(offset) * body.getAngularVelocity();
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

    const float targetImpulseMagnitude = tangentialSpeed / denominator;
    float frictionLimit = dynamicFriction * normalImpulseMagnitude;
    if (targetImpulseMagnitude <= staticFriction * normalImpulseMagnitude)
    {
        frictionLimit = targetImpulseMagnitude;
    }

    const float impulseMagnitude = -std::min(targetImpulseMagnitude, frictionLimit);
    const Vector2 impulse = tangent * impulseMagnitude;
    bodyA.setVelocity(bodyA.getVelocity() - impulse * bodyA.getInverseMass());
    bodyB.setVelocity(bodyB.getVelocity() + impulse * bodyB.getInverseMass());
    bodyA.setAngularVelocity(bodyA.getAngularVelocity() - Vector2::cross(offsetA, impulse) * bodyA.getInverseMomentOfInertia());
    bodyB.setAngularVelocity(bodyB.getAngularVelocity() + Vector2::cross(offsetB, impulse) * bodyB.getInverseMomentOfInertia());
}

bool resolveDynamicCircleRectCollisionInternal(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, const Contact2D &contactA) 
{
    const bool shapePairIsCircleRect =
        (dynamic_cast<CircleShape *>(bodyA.getShape()) && dynamic_cast<RectShape *>(bodyB.getShape())) ||
        (dynamic_cast<RectShape *>(bodyA.getShape()) && dynamic_cast<CircleShape *>(bodyB.getShape()));
    if (!shapePairIsCircleRect || bodyA.isStatic() || bodyB.isStatic() || contactA.penetration <= 0.0f)
    {
        return false;
    }

    const Vector2 normal = contactA.normal;
    if (normal.lengthSquared() <= 0.0f)
    {
        return false;
    }

    const float inverseMassSum = bodyA.getInverseMass() + bodyB.getInverseMass();
    if (inverseMassSum <= 0.0f)
    {
        return false;
    }

    const float correctedPenetration = std::max(0.0f, contactA.penetration - kPositionCorrectionSlop2D) * kPositionCorrectionPercent2D;
    bodyA.setPosition(bodyA.getPosition() - normal * (correctedPenetration * (bodyA.getInverseMass() / inverseMassSum)));
    bodyB.setPosition(bodyB.getPosition() + normal * (correctedPenetration * (bodyB.getInverseMass() / inverseMassSum)));

    const Vector2 contactVelocityA = getContactVelocity(bodyA, contactA.contactPoint);
    const Vector2 contactVelocityB = getContactVelocity(bodyB, contactA.contactPoint);
    const Vector2 relativeVelocity = contactVelocityB - contactVelocityA;
    const float velocityAlongNormal = relativeVelocity.dot(normal);
    if (velocityAlongNormal > 0.0f)
    {
        return true;
    }

    const float combinedRestitution = std::min(bodyA.getSurfaceMaterial().restitution, bodyB.getSurfaceMaterial().restitution);
    const Vector2 offsetA = contactA.contactPoint - getBodyCenter(bodyA);
    const Vector2 offsetB = contactA.contactPoint - getBodyCenter(bodyB);
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

    const float normalImpulseMagnitude = -(1.0f + combinedRestitution) * velocityAlongNormal / denominator;
    const Vector2 impulse = normal * normalImpulseMagnitude;
    bodyA.setVelocity(bodyA.getVelocity() - impulse * bodyA.getInverseMass());
    bodyB.setVelocity(bodyB.getVelocity() + impulse * bodyB.getInverseMass());
    bodyA.setAngularVelocity(bodyA.getAngularVelocity() - Vector2::cross(offsetA, impulse) * bodyA.getInverseMomentOfInertia());
    bodyB.setAngularVelocity(bodyB.getAngularVelocity() + Vector2::cross(offsetB, impulse) * bodyB.getInverseMomentOfInertia());

    applyDynamicFrictionImpulse(bodyA, bodyB, contactA, normalImpulseMagnitude);
    return true;
}

}

void CollisionSolver2D::solve(Scene2D &scene, std::vector<Manifold2D> &manifolds) const
{
    manifolds.clear();
    auto &bodies = scene.getBodies();

    for (size_t i = 0; i < bodies.size(); ++i)
    {
        for (size_t j = i + 1; j < bodies.size(); ++j)
        {
            Shape *shapeA = bodies[i]->getShape();
            Shape *shapeB = bodies[j]->getShape();
            if (!shapeA || !shapeB || !bodies[i]->isCollisionEnabled() || !bodies[j]->isCollisionEnabled())
                continue;

            if (!shapeA->collidesWith(*shapeB, bodies[i]->getPosition(), bodies[j]->getPosition()))
                continue;

            Manifold2D manifold;
            if (!buildManifold(*bodies[i], *bodies[j], manifold))
                continue;

            manifolds.push_back(manifold);

            Contact2D contactA;
            Contact2D contactB;
            buildContactsFromManifold(manifolds.back(), contactA, contactB);

            if (resolveDynamicCircleCollision(contactA, contactB))
                continue;

            if (resolveDynamicCircleRectCollisionInternal(*bodies[i], *bodies[j], contactA))
                continue;

            bodies[i]->onCollision(contactA);
            bodies[j]->onCollision(contactB);
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
        Vector2 delta = bodyB.getPosition() - borderCircleA->getCenter();
        float distanceSquared = delta.lengthSquared();
        float innerRadius = borderCircleA->getRadius() - circleB->getRadius();
        if (distanceSquared < innerRadius * innerRadius)
            return false;

        float distance = std::sqrt(std::max(distanceSquared, 0.0001f));
        Vector2 normal = delta / distance;
        float penetration = distance - innerRadius;
        manifold.bodyA = &bodyA;
        manifold.bodyB = &bodyB;
        manifold.normal = normal * -1.0f;
        manifold.penetration = penetration;
        manifold.contactPoints[0] = borderCircleA->getCenter() + normal * innerRadius;
        manifold.contactCount = 1;
        return true;
    }

    if (circleA && circleB)
    {
        Vector2 delta = bodyB.getPosition() - bodyA.getPosition();
        float distanceSquared = delta.lengthSquared();
        float radiusSum = circleA->getRadius() + circleB->getRadius();
        if (distanceSquared > radiusSum * radiusSum)
            return false;

        Vector2 normal = Vector2(1.0f, 0.0f);
        float distance = 0.0f;
        if (distanceSquared > 0.0001f)
        {
            distance = std::sqrt(distanceSquared);
            normal = delta / distance;
        }
        float penetration = radiusSum - distance;

        manifold.bodyA = &bodyA;
        manifold.bodyB = &bodyB;
        manifold.normal = normal;
        manifold.penetration = penetration;
        manifold.contactPoints[0] = bodyA.getPosition() + normal * circleA->getRadius();
        manifold.contactCount = 1;
        return true;
    }

    if (circleA && borderCircleB)
    {
        Vector2 delta = bodyA.getPosition() - borderCircleB->getCenter();
        float distanceSquared = delta.lengthSquared();
        float innerRadius = borderCircleB->getRadius() - circleA->getRadius();
        if (distanceSquared < innerRadius * innerRadius)
            return false;

        float distance = std::sqrt(std::max(distanceSquared, 0.0001f));
        Vector2 normal = delta / distance;
        float penetration = distance - innerRadius;
        manifold.bodyA = &bodyA;
        manifold.bodyB = &bodyB;
        manifold.normal = normal;
        manifold.penetration = penetration;
        manifold.contactPoints[0] = borderCircleB->getCenter() + normal * innerRadius;
        manifold.contactCount = 1;
        return true;
    }

    auto *rectA = dynamic_cast<RectShape *>(bodyA.getShape());
    auto *rectB = dynamic_cast<RectShape *>(bodyB.getShape());
    auto buildCircleRectManifold = [](PhysicsBody2D &circleBody, CircleShape &circle, PhysicsBody2D &rectBody, RectShape &rect, Manifold2D &manifold, bool circleIsBodyA) -> bool
    {
        const Vector2 &circlePosition = circleBody.getPosition();
        const Vector2 &rectPosition = rectBody.getPosition();
        float closestX = std::max(rectPosition.x, std::min(circlePosition.x, rectPosition.x + rect.getWidth()));
        float closestY = std::max(rectPosition.y, std::min(circlePosition.y, rectPosition.y + rect.getHeight()));
        Vector2 delta(circlePosition.x - closestX, circlePosition.y - closestY);
        float distanceSquared = delta.lengthSquared();
        float radius = circle.getRadius();
        if (distanceSquared > radius * radius)
            return false;

        Vector2 normal = Vector2::zero();
        if (distanceSquared > 0.0f)
        {
            normal = delta.normalized();
        }
        else
        {
            float leftPen = std::abs(circlePosition.x - rectPosition.x);
            float rightPen = std::abs((rectPosition.x + rect.getWidth()) - circlePosition.x);
            float topPen = std::abs(circlePosition.y - rectPosition.y);
            float bottomPen = std::abs((rectPosition.y + rect.getHeight()) - circlePosition.y);
            float minPen = std::min(std::min(leftPen, rightPen), std::min(topPen, bottomPen));

            if (minPen == leftPen)
                normal = Vector2(-1.0f, 0.0f);
            else if (minPen == rightPen)
                normal = Vector2(1.0f, 0.0f);
            else if (minPen == topPen)
                normal = Vector2(0.0f, -1.0f);
            else
                normal = Vector2(0.0f, 1.0f);
        }

        float distance = std::sqrt(std::max(distanceSquared, 0.0f));
        float penetration = radius - distance;
        manifold.bodyA = circleIsBodyA ? &circleBody : &rectBody;
        manifold.bodyB = circleIsBodyA ? &rectBody : &circleBody;
        manifold.normal = circleIsBodyA ? normal : normal * -1.0f;
        manifold.penetration = penetration;
        manifold.contactPoints[0] = Vector2(closestX, closestY);
        manifold.contactCount = 1;
        return true;
    };

    if (circleA && rectB)
        return buildCircleRectManifold(bodyA, *circleA, bodyB, *rectB, manifold, true);

    if (rectA && circleB)
        return buildCircleRectManifold(bodyB, *circleB, bodyA, *rectA, manifold, false);

    if (borderCircleA && rectB)
    {
        const BorderCircleRectContact2D contact = buildBorderCircleRectContact(*borderCircleA, *rectB, bodyB.getPosition(), bodyB.getRotation());
        if (!contact.intersects)
            return false;

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
        const BorderCircleRectContact2D contact = buildBorderCircleRectContact(*borderCircleB, *rectA, bodyA.getPosition(), bodyA.getRotation());
        if (!contact.intersects)
            return false;

        manifold.bodyA = &bodyA;
        manifold.bodyB = &bodyB;
        manifold.normal = contact.normal;
        manifold.penetration = contact.penetration;
        manifold.contactPoints[0] = contact.contactPoint;
        manifold.contactCount = 1;
        return true;
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
    auto *bodyA = contactA.self;
    auto *bodyB = contactB.self;
    if (!bodyA || !bodyB || bodyA->isStatic() || bodyB->isStatic())
        return false;

    auto *circleA = dynamic_cast<CircleShape *>(bodyA->getShape());
    auto *circleB = dynamic_cast<CircleShape *>(bodyB->getShape());
    if (!circleA || !circleB || contactA.penetration <= 0.0f)
        return false;

    const Vector2 normal = contactA.normal;
    const float totalMass = bodyA->getMass() + bodyB->getMass();
    if (normal.lengthSquared() <= 0.0f || totalMass <= 0.0f)
        return false;

    const float correctedPenetration = std::max(0.0f, contactA.penetration - kPositionCorrectionSlop2D) * kPositionCorrectionPercent2D;
    bodyA->setPosition(bodyA->getPosition() - normal * (correctedPenetration * (bodyB->getMass() / totalMass)));
    bodyB->setPosition(bodyB->getPosition() + normal * (correctedPenetration * (bodyA->getMass() / totalMass)));

    const Vector2 contactPoint = contactA.contactPoint;
    const Vector2 contactOffsetA = contactPoint - getBodyCenter(*bodyA);
    const Vector2 contactOffsetB = contactPoint - getBodyCenter(*bodyB);
    const Vector2 contactVelocityA = getContactVelocity(*bodyA, contactPoint);
    const Vector2 contactVelocityB = getContactVelocity(*bodyB, contactPoint);
    const Vector2 relativeVelocity = contactVelocityB - contactVelocityA;
    const float velocityAlongNormal = relativeVelocity.dot(normal);
    if (velocityAlongNormal > 0.0f)
        return true;

    const float combinedRestitution = std::min(bodyA->getSurfaceMaterial().restitution, bodyB->getSurfaceMaterial().restitution);
    const float radiusCrossNormalA = Vector2::cross(contactOffsetA, normal);
    const float radiusCrossNormalB = Vector2::cross(contactOffsetB, normal);
    const float denominator =
        bodyA->getInverseMass() +
        bodyB->getInverseMass() +
        radiusCrossNormalA * radiusCrossNormalA * bodyA->getInverseMomentOfInertia() +
        radiusCrossNormalB * radiusCrossNormalB * bodyB->getInverseMomentOfInertia();
    if (denominator <= 0.0f)
    {
        return true;
    }

    const float impulseMagnitude = -(1.0f + combinedRestitution) * velocityAlongNormal / denominator;

    const Vector2 impulse = normal * impulseMagnitude;
    bodyA->setVelocity(bodyA->getVelocity() - impulse * bodyA->getInverseMass());
    bodyB->setVelocity(bodyB->getVelocity() + impulse * bodyB->getInverseMass());
    bodyA->setAngularVelocity(bodyA->getAngularVelocity() - Vector2::cross(contactOffsetA, impulse) * bodyA->getInverseMomentOfInertia());
    bodyB->setAngularVelocity(bodyB->getAngularVelocity() + Vector2::cross(contactOffsetB, impulse) * bodyB->getInverseMomentOfInertia());

    applyDynamicFrictionImpulse(*bodyA, *bodyB, contactA, impulseMagnitude);
    return true;
}
