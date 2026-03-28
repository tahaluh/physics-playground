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
            if (!shapeA || !shapeB)
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

    bodyA->setPosition(bodyA->getPosition() - normal * (contactA.penetration * (bodyB->getMass() / totalMass)));
    bodyB->setPosition(bodyB->getPosition() + normal * (contactA.penetration * (bodyA->getMass() / totalMass)));

    Vector2 relativeVelocity = bodyB->getVelocity() - bodyA->getVelocity();
    float velocityAlongNormal = relativeVelocity.dot(normal);
    if (velocityAlongNormal > 0.0f)
        return true;

    const float combinedRestitution = std::min(bodyA->getMaterial().restitution, bodyB->getMaterial().restitution);
    float impulseMagnitude = -(1.0f + combinedRestitution) * velocityAlongNormal;
    impulseMagnitude /= (1.0f / bodyA->getMass()) + (1.0f / bodyB->getMass());

    Vector2 impulse = normal * impulseMagnitude;
    bodyA->setVelocity(bodyA->getVelocity() - impulse / bodyA->getMass());
    bodyB->setVelocity(bodyB->getVelocity() + impulse / bodyB->getMass());
    return true;
}
