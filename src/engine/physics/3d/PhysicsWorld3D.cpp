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

            const float outwardSpeed = dynamicCandidate->getVelocity().dot(normal);
            if (outwardSpeed > 0.0f)
            {
                const float restitution = dynamicCandidate->getMaterial().restitution;
                dynamicCandidate->setVelocity(
                    dynamicCandidate->getVelocity() - normal * ((1.0f + restitution) * outwardSpeed));
            }
        }
    }
}
