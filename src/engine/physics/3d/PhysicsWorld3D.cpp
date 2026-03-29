#include "engine/physics/3d/PhysicsWorld3D.h"

#include "engine/physics/3d/PhysicsBody3D.h"
#include "engine/physics/3d/shapes/BoxCollider3D.h"
#include "engine/physics/3d/shapes/SphereShape3D.h"
#include "engine/scene/3d/PhysicsScene3D.h"

namespace
{
constexpr float kBoxAngularImpulseScale = 0.32f;

struct OrientedBoxBasis
{
    Vector3 axisX;
    Vector3 axisY;
    Vector3 axisZ;
};

OrientedBoxBasis getBodyBasis(const PhysicsBody3D &body)
{
    const Matrix4 rotation = body.getRotationMatrix();
    return {
        rotation.transformVector(Vector3::right()).normalized(),
        rotation.transformVector(Vector3::up()).normalized(),
        rotation.transformVector(Vector3::forward()).normalized()};
}

Vector3 getClosestPointOnBox(const PhysicsBody3D &boxBody, const BoxCollider3D &box, const Vector3 &point)
{
    const OrientedBoxBasis basis = getBodyBasis(boxBody);
    const Vector3 delta = point - boxBody.getPosition();
    const Vector3 halfExtents = box.getHalfExtents();
    const float localX = Vector3::clamp(delta.dot(basis.axisX), -halfExtents.x, halfExtents.x);
    const float localY = Vector3::clamp(delta.dot(basis.axisY), -halfExtents.y, halfExtents.y);
    const float localZ = Vector3::clamp(delta.dot(basis.axisZ), -halfExtents.z, halfExtents.z);
    return boxBody.getPosition() + basis.axisX * localX + basis.axisY * localY + basis.axisZ * localZ;
}

Vector3 getBoxSupportPoint(const PhysicsBody3D &boxBody, const BoxCollider3D &box, const Vector3 &direction)
{
    const OrientedBoxBasis basis = getBodyBasis(boxBody);
    const Vector3 halfExtents = box.getHalfExtents();
    const float sx = direction.dot(basis.axisX) >= 0.0f ? halfExtents.x : -halfExtents.x;
    const float sy = direction.dot(basis.axisY) >= 0.0f ? halfExtents.y : -halfExtents.y;
    const float sz = direction.dot(basis.axisZ) >= 0.0f ? halfExtents.z : -halfExtents.z;
    return boxBody.getPosition() + basis.axisX * sx + basis.axisY * sy + basis.axisZ * sz;
}

void applyFrictionImpulse3D(
    PhysicsBody3D &bodyA,
    PhysicsBody3D &bodyB,
    const Vector3 &normal,
    const Vector3 &contactOffsetA,
    const Vector3 &contactOffsetB)
{
    const Vector3 contactVelocityA = bodyA.getVelocity() + bodyA.getAngularVelocity().cross(contactOffsetA);
    const Vector3 contactVelocityB = bodyB.getVelocity() + bodyB.getAngularVelocity().cross(contactOffsetB);
    const Vector3 relativeVelocity = contactVelocityB - contactVelocityA;
    const float normalSpeed = relativeVelocity.dot(normal);
    const Vector3 tangentialVelocity = relativeVelocity - normal * normalSpeed;
    const float tangentialSpeed = tangentialVelocity.length();
    if (tangentialSpeed <= 0.0f)
    {
        return;
    }

    const float frictionStrength = Vector3::clamp(
        (bodyA.getSurfaceMaterial().friction + bodyB.getSurfaceMaterial().friction) * 0.5f,
        0.0f,
        1.0f);
    if (frictionStrength <= 0.0f)
    {
        return;
    }

    const Vector3 tangent = tangentialVelocity / tangentialSpeed;
    const Vector3 radiusCrossTangentA = contactOffsetA.cross(tangent);
    const Vector3 radiusCrossTangentB = contactOffsetB.cross(tangent);
    const float denominator =
        bodyA.getInverseMass() +
        bodyB.getInverseMass() +
        radiusCrossTangentA.lengthSquared() * bodyA.getInverseMomentOfInertia() +
        radiusCrossTangentB.lengthSquared() * bodyB.getInverseMomentOfInertia();
    if (denominator <= 0.0f)
    {
        return;
    }

    const float impulseMagnitude = -(tangentialSpeed / denominator) * frictionStrength;
    const Vector3 impulse = tangent * impulseMagnitude;
    bodyA.setVelocity(bodyA.getVelocity() - impulse * bodyA.getInverseMass());
    bodyB.setVelocity(bodyB.getVelocity() + impulse * bodyB.getInverseMass());
    bodyA.setAngularVelocity(bodyA.getAngularVelocity() - contactOffsetA.cross(impulse) * bodyA.getInverseMomentOfInertia());
    bodyB.setAngularVelocity(bodyB.getAngularVelocity() + contactOffsetB.cross(impulse) * bodyB.getInverseMomentOfInertia());
}

void steerSphereTowardRolling(PhysicsBody3D &body, const SphereShape3D &sphere, const Vector3 &normal, float frictionStrength)
{
    if (sphere.getRadius() <= 0.0f || frictionStrength <= 0.0f)
    {
        return;
    }

    const Vector3 tangentialLinearVelocity = body.getVelocity() - normal * body.getVelocity().dot(normal);
    const float tangentialSpeed = tangentialLinearVelocity.length();
    if (tangentialSpeed <= 0.0001f)
    {
        return;
    }

    const Vector3 desiredAngularVelocity = tangentialLinearVelocity.cross(normal) / sphere.getRadius();
    const float blend = Vector3::clamp(0.35f + frictionStrength * 0.65f, 0.0f, 1.0f);
    body.setAngularVelocity(body.getAngularVelocity() * (1.0f - blend) + desiredAngularVelocity * blend);
}

void solveSphereBoxCollision(PhysicsBody3D &sphereBody, const SphereShape3D &sphere, PhysicsBody3D &boxBody, const BoxCollider3D &box)
{
    const Vector3 closestPoint = getClosestPointOnBox(boxBody, box, sphereBody.getPosition());
    Vector3 normal = closestPoint - sphereBody.getPosition();
    float distance = normal.length();
    const float penetration = sphere.getRadius() - distance;
    if (penetration <= 0.0f)
    {
        return;
    }

    if (distance <= 0.0001f)
    {
        normal = (boxBody.getPosition() - sphereBody.getPosition()).normalized();
        if (normal.lengthSquared() == 0.0f)
        {
            normal = Vector3::up();
        }
        distance = 0.0f;
    }
    else
    {
        normal /= distance;
    }

    const float inverseMassSum = sphereBody.getInverseMass() + boxBody.getInverseMass();
    if (inverseMassSum <= 0.0f)
    {
        return;
    }

    const Vector3 correction = normal * (penetration / inverseMassSum);
    sphereBody.setPosition(sphereBody.getPosition() - correction * sphereBody.getInverseMass());
    boxBody.setPosition(boxBody.getPosition() + correction * boxBody.getInverseMass());

    const Vector3 contactPoint = getClosestPointOnBox(boxBody, box, sphereBody.getPosition());
    const Vector3 contactOffsetSphere = contactPoint - sphereBody.getCenterOfMassWorldPosition();
    const Vector3 contactOffsetBox = contactPoint - boxBody.getCenterOfMassWorldPosition();
    const Vector3 contactVelocitySphere = sphereBody.getVelocity() + sphereBody.getAngularVelocity().cross(contactOffsetSphere);
    const Vector3 contactVelocityBox = boxBody.getVelocity() + boxBody.getAngularVelocity().cross(contactOffsetBox);
    const Vector3 relativeVelocity = contactVelocityBox - contactVelocitySphere;
    const float normalSpeed = relativeVelocity.dot(normal);
    if (normalSpeed >= 0.0f)
    {
        return;
    }

    const float restitution = (sphereBody.getSurfaceMaterial().restitution + boxBody.getSurfaceMaterial().restitution) * 0.5f;
    const Vector3 radiusCrossNormalSphere = contactOffsetSphere.cross(normal);
    const Vector3 radiusCrossNormalBox = contactOffsetBox.cross(normal);
    const float denominator =
        inverseMassSum +
        radiusCrossNormalSphere.lengthSquared() * sphereBody.getInverseMomentOfInertia() +
        radiusCrossNormalBox.lengthSquared() * boxBody.getInverseMomentOfInertia();
    if (denominator <= 0.0f)
    {
        return;
    }

    const float impulseMagnitude = -(1.0f + restitution) * normalSpeed / denominator;
    const Vector3 impulse = normal * impulseMagnitude;
    sphereBody.setVelocity(sphereBody.getVelocity() - impulse * sphereBody.getInverseMass());
    boxBody.setVelocity(boxBody.getVelocity() + impulse * boxBody.getInverseMass());
    sphereBody.setAngularVelocity(sphereBody.getAngularVelocity() - contactOffsetSphere.cross(impulse) * sphereBody.getInverseMomentOfInertia());
    boxBody.setAngularVelocity(
        boxBody.getAngularVelocity() +
        contactOffsetBox.cross(impulse) * boxBody.getInverseMomentOfInertia() * kBoxAngularImpulseScale);

    applyFrictionImpulse3D(sphereBody, boxBody, normal, contactOffsetSphere, contactOffsetBox);
}

void solveDynamicSphereSphereCollisions(PhysicsScene3D &scene)
{
    auto &bodies = scene.getBodies();
    for (std::size_t i = 0; i < bodies.size(); ++i)
    {
        PhysicsBody3D *bodyA = bodies[i].get();
        if (!bodyA || bodyA->isStatic())
        {
            continue;
        }

        const auto *shapeA = dynamic_cast<const SphereShape3D *>(bodyA->getShape());
        if (!shapeA)
        {
            continue;
        }

        for (std::size_t j = i + 1; j < bodies.size(); ++j)
        {
            PhysicsBody3D *bodyB = bodies[j].get();
            if (!bodyB || bodyB->isStatic())
            {
                continue;
            }

            const auto *shapeB = dynamic_cast<const SphereShape3D *>(bodyB->getShape());
            if (!shapeB)
            {
                continue;
            }

            Vector3 delta = bodyB->getPosition() - bodyA->getPosition();
            float distance = delta.length();
            const float minimumDistance = shapeA->getRadius() + shapeB->getRadius();
            if (distance >= minimumDistance)
            {
                continue;
            }

            const Vector3 normal = distance > 0.0f ? delta / distance : Vector3(1.0f, 0.0f, 0.0f);
            const float penetration = minimumDistance - distance;
            const float inverseMassSum = bodyA->getInverseMass() + bodyB->getInverseMass();
            if (inverseMassSum <= 0.0f)
            {
                continue;
            }

            const Vector3 correction = normal * (penetration / inverseMassSum);
            bodyA->setPosition(bodyA->getPosition() - correction * bodyA->getInverseMass());
            bodyB->setPosition(bodyB->getPosition() + correction * bodyB->getInverseMass());

            const Vector3 contactPoint = bodyA->getPosition() + normal * shapeA->getRadius();
            const Vector3 contactOffsetA = contactPoint - bodyA->getCenterOfMassWorldPosition();
            const Vector3 contactOffsetB = contactPoint - bodyB->getCenterOfMassWorldPosition();
            const Vector3 contactVelocityA = bodyA->getVelocity() + bodyA->getAngularVelocity().cross(contactOffsetA);
            const Vector3 contactVelocityB = bodyB->getVelocity() + bodyB->getAngularVelocity().cross(contactOffsetB);
            const Vector3 relativeVelocity = contactVelocityB - contactVelocityA;
            const float normalSpeed = relativeVelocity.dot(normal);
            if (normalSpeed >= 0.0f)
            {
                continue;
            }

            const float restitution = (bodyA->getSurfaceMaterial().restitution + bodyB->getSurfaceMaterial().restitution) * 0.5f;
            const Vector3 radiusCrossNormalA = contactOffsetA.cross(normal);
            const Vector3 radiusCrossNormalB = contactOffsetB.cross(normal);
            const float denominator =
                inverseMassSum +
                radiusCrossNormalA.lengthSquared() * bodyA->getInverseMomentOfInertia() +
                radiusCrossNormalB.lengthSquared() * bodyB->getInverseMomentOfInertia();
            if (denominator <= 0.0f)
            {
                continue;
            }

            const float impulseMagnitude = -(1.0f + restitution) * normalSpeed / denominator;
            const Vector3 impulse = normal * impulseMagnitude;
            bodyA->setVelocity(bodyA->getVelocity() - impulse * bodyA->getInverseMass());
            bodyB->setVelocity(bodyB->getVelocity() + impulse * bodyB->getInverseMass());
            bodyA->setAngularVelocity(bodyA->getAngularVelocity() - contactOffsetA.cross(impulse) * bodyA->getInverseMomentOfInertia());
            bodyB->setAngularVelocity(bodyB->getAngularVelocity() + contactOffsetB.cross(impulse) * bodyB->getInverseMomentOfInertia());

            applyFrictionImpulse3D(*bodyA, *bodyB, normal, contactOffsetA, contactOffsetB);
        }
    }
}

void solveDynamicSphereBoxCollisions(PhysicsScene3D &scene)
{
    auto &bodies = scene.getBodies();
    for (std::size_t i = 0; i < bodies.size(); ++i)
    {
        PhysicsBody3D *bodyA = bodies[i].get();
        if (!bodyA || bodyA->isStatic())
        {
            continue;
        }

        for (std::size_t j = i + 1; j < bodies.size(); ++j)
        {
            PhysicsBody3D *bodyB = bodies[j].get();
            if (!bodyB || bodyB->isStatic())
            {
                continue;
            }

            const auto *sphereA = dynamic_cast<const SphereShape3D *>(bodyA->getShape());
            const auto *boxA = dynamic_cast<const BoxCollider3D *>(bodyA->getShape());
            const auto *sphereB = dynamic_cast<const SphereShape3D *>(bodyB->getShape());
            const auto *boxB = dynamic_cast<const BoxCollider3D *>(bodyB->getShape());

            if (sphereA && boxB)
            {
                solveSphereBoxCollision(*bodyA, *sphereA, *bodyB, *boxB);
            }
            else if (boxA && sphereB)
            {
                solveSphereBoxCollision(*bodyB, *sphereB, *bodyA, *boxA);
            }
        }
    }
}
}

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

void PhysicsWorld3D::setAngularStopThreshold(float newAngularStopThreshold)
{
    angularStopThreshold = newAngularStopThreshold;
}

float PhysicsWorld3D::getAngularStopThreshold() const
{
    return angularStopThreshold;
}

void PhysicsWorld3D::step(PhysicsScene3D &scene, float dt) const
{
    applyGlobalForces(scene);
    integrateBodies(scene, dt);
    solveDynamicSphereSphereCollisions(scene);
    solveDynamicSphereBoxCollisions(scene);
    solveBoundaryCollisions(scene);
}

void PhysicsWorld3D::applyGlobalForces(PhysicsScene3D &scene) const
{
    for (auto &body : scene.getBodies())
    {
        if (body->isStatic() || !body->getRigidBodySettings().useGravity)
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
            const auto *dynamicBox = dynamic_cast<const BoxCollider3D *>(dynamicCandidate->getShape());
            if (!dynamicShape && !dynamicBox)
            {
                continue;
            }

            Vector3 contactPoint = Vector3::zero();
            Vector3 normal = Vector3::up();
            float penetration = 0.0f;

            if (dynamicShape)
            {
                const Vector3 relative = dynamicCandidate->getPosition() - borderCandidate->getPosition();
                const float distance = relative.length();
                const float maximumDistance = borderShape->getRadius() - dynamicShape->getRadius();
                if (distance <= maximumDistance)
                {
                    continue;
                }

                normal = distance > 0.0f ? relative / distance : Vector3::up();
                penetration = distance - maximumDistance;
                contactPoint = dynamicCandidate->getPosition() - normal * dynamicShape->getRadius();
            }
            else
            {
            const Vector3 supportPoint = getBoxSupportPoint(*dynamicCandidate, *dynamicBox, dynamicCandidate->getPosition() - borderCandidate->getPosition());
                const Vector3 supportRelative = supportPoint - borderCandidate->getPosition();
                const float supportDistance = supportRelative.length();
                if (supportDistance <= borderShape->getRadius())
                {
                    continue;
                }

                normal = supportDistance > 0.0f ? supportRelative / supportDistance : Vector3::up();
                penetration = supportDistance - borderShape->getRadius();
                contactPoint = supportPoint;
            }

            dynamicCandidate->setPosition(dynamicCandidate->getPosition() - normal * penetration);

            const Vector3 contactOffset = contactPoint - dynamicCandidate->getCenterOfMassWorldPosition();
            const Vector3 contactVelocity =
                dynamicCandidate->getVelocity() + dynamicCandidate->getAngularVelocity().cross(contactOffset);
            const float outwardSpeed = contactVelocity.dot(normal);
            if (outwardSpeed > 0.0f)
            {
                const float restitution = dynamicCandidate->getSurfaceMaterial().restitution;
                dynamicCandidate->setVelocity(
                    dynamicCandidate->getVelocity() - normal * ((1.0f + restitution) * outwardSpeed));
            }

            const float contactNormalSpeed = contactVelocity.dot(normal);
            const Vector3 tangentialVelocity = contactVelocity - normal * contactNormalSpeed;
            const float tangentialSpeed = tangentialVelocity.length();
            const float surfaceFriction = dynamicCandidate->getSurfaceMaterial().friction;
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
                    const float angularScale = dynamicBox ? kBoxAngularImpulseScale : 1.0f;
                    dynamicCandidate->setAngularVelocity(
                        dynamicCandidate->getAngularVelocity() +
                        contactOffset.cross(impulse) * dynamicCandidate->getInverseMomentOfInertia() * angularScale);
                }
            }

            if (dynamicShape)
            {
                steerSphereTowardRolling(*dynamicCandidate, *dynamicShape, normal, frictionStrength);
            }

            if (stopThreshold > 0.0f &&
                dynamicCandidate->getVelocity().length() < stopThreshold &&
                dynamicCandidate->getAngularVelocity().length() < angularStopThreshold)
            {
                dynamicCandidate->setVelocity(Vector3::zero());
                dynamicCandidate->setAngularVelocity(Vector3::zero());
            }

        }
    }
}
