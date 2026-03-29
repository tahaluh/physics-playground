#include "engine/physics/3d/PhysicsWorld3D.h"

#include "engine/physics/3d/PhysicsBody3D.h"
#include "engine/physics/3d/shapes/SphereShape3D.h"
#include "engine/scene/3d/PhysicsScene3D.h"

void PhysicsWorld3D::setGravity(const Vector3 &newGravity)
{
    gravity = newGravity;
}

const Vector3 &PhysicsWorld3D::getGravity() const
{
    return gravity;
}

void PhysicsWorld3D::setStopThreshold(float newStopThreshold)
{
    stopThreshold = newStopThreshold;
}

float PhysicsWorld3D::getStopThreshold() const
{
    return stopThreshold;
}

void PhysicsWorld3D::step(PhysicsScene3D &scene, float dt) const
{
    applyGlobalForces(scene);
    integrateBodies(scene, dt);
    solveBoundaryCollisions(scene);
}

void PhysicsWorld3D::applyGlobalForces(PhysicsScene3D &scene) const
{
    for (auto &body : scene.getBodies())
    {
        if (body->isStatic() || !body->getMaterial().useGravity)
        {
            continue;
        }

        body->applyForce(gravity * body->getMass());
    }
}

void PhysicsWorld3D::integrateBodies(PhysicsScene3D &scene, float dt) const
{
    for (auto &body : scene.getBodies())
    {
        body->integrate(dt);
    }
}

void PhysicsWorld3D::solveBoundaryCollisions(PhysicsScene3D &scene) const
{
    auto &bodies = scene.getBodies();
    for (const auto &borderCandidate : bodies)
    {
        if (!borderCandidate->isStatic())
        {
            continue;
        }

        const auto *borderShape = dynamic_cast<const SphereShape3D *>(borderCandidate->getShape());
        if (!borderShape)
        {
            continue;
        }

        for (const auto &dynamicCandidate : bodies)
        {
            if (dynamicCandidate->isStatic())
            {
                continue;
            }

            const auto *dynamicShape = dynamic_cast<const SphereShape3D *>(dynamicCandidate->getShape());
            if (!dynamicShape)
            {
                continue;
            }

            const Vector3 relative = dynamicCandidate->getPosition() - borderCandidate->getPosition();
            const float distance = relative.length();
            const float maximumDistance = borderShape->getRadius() - dynamicShape->getRadius();
            if (distance <= maximumDistance)
            {
                continue;
            }

            const Vector3 normal = distance > 0.0f ? relative / distance : Vector3::up();
            dynamicCandidate->setPosition(borderCandidate->getPosition() + normal * maximumDistance);

            const float dynamicRadius = dynamicShape->getRadius();
            const Vector3 contactOffset = normal * dynamicRadius;
            const Vector3 contactVelocity =
                dynamicCandidate->getVelocity() + dynamicCandidate->getAngularVelocity().cross(contactOffset);
            const float outwardSpeed = dynamicCandidate->getVelocity().dot(normal);
            if (outwardSpeed > 0.0f)
            {
                const float restitution = dynamicCandidate->getMaterial().restitution;
                dynamicCandidate->setVelocity(
                    dynamicCandidate->getVelocity() - normal * ((1.0f + restitution) * outwardSpeed));
            }

            const float contactNormalSpeed = contactVelocity.dot(normal);
            const Vector3 tangentialVelocity = contactVelocity - normal * contactNormalSpeed;
            const float tangentialSpeed = tangentialVelocity.length();
            const float surfaceFriction = dynamicCandidate->getMaterial().surfaceFriction;
            const float frictionStrength = Vector3::clamp(surfaceFriction, 0.0f, 1.0f);
            if (surfaceFriction > 0.0f && tangentialSpeed > 0.0f)
            {
                const Vector3 tangent = tangentialVelocity / tangentialSpeed;
                const Vector3 radiusCrossTangent = contactOffset.cross(tangent);
                const float denominator =
                    dynamicCandidate->getInverseMass() +
                    radiusCrossTangent.lengthSquared() * dynamicCandidate->getInverseMomentOfInertia();
                if (denominator > 0.0f)
                {
                    const float impulseMagnitude = -(tangentialSpeed / denominator) * frictionStrength;
                    const Vector3 impulse = tangent * impulseMagnitude;
                    dynamicCandidate->setVelocity(
                        dynamicCandidate->getVelocity() + impulse * dynamicCandidate->getInverseMass());
                    dynamicCandidate->setAngularVelocity(
                        dynamicCandidate->getAngularVelocity() +
                        contactOffset.cross(impulse) * dynamicCandidate->getInverseMomentOfInertia());
                }
            }

            if (stopThreshold > 0.0f && dynamicCandidate->getVelocity().length() < stopThreshold)
            {
                dynamicCandidate->setVelocity(Vector3::zero());
                dynamicCandidate->setAngularVelocity(Vector3::zero());
            }

        }
    }
}
