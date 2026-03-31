#include "engine/physics/PhysicsSystem.h"

#include <algorithm>
#include "engine/physics/BoxCollider.h"
#include "engine/physics/CollisionTests.h"
#include "engine/physics/GpuBroadPhase.h"
#include "engine/physics/PlaneCollider.h"
#include "engine/physics/SapBroadPhase.h"
#include "engine/scene/objects/BodyObject.h"

namespace
{
    constexpr float kSleepLinearSpeedThreshold = 0.02f;
    constexpr float kSleepAngularSpeedThreshold = 0.02f;
    constexpr float kSleepDelaySeconds = 0.6f;
    constexpr float kPenetrationSlop = 0.005f;
    constexpr float kPenetrationCorrectionPercent = 0.8f;
    constexpr float kRestitutionVelocityThreshold = 2.0f;
    constexpr float kWakeVelocityThreshold = 0.25f;
    constexpr float kRestingContactVelocityThreshold = 0.16f;
    constexpr float kRestingTangentVelocityThreshold = 0.1f;
    constexpr float kRestingAngularSpeedThreshold = 0.1f;
    constexpr float kSupportContactNormalThreshold = 0.55f;
    constexpr int kSolverIterations = 6;
    constexpr float kLinearDampingPerSecond = 0.985f;
    constexpr float kAngularDampingPerSecond = 0.98f;
    const Vector3 kGravity(0.0f, -9.81f, 0.0f);
    constexpr float kPersistentContactDistanceThreshold = 0.35f;
    constexpr float kPersistentNormalDotThreshold = 0.85f;

    CollisionPoints invertCollision(const CollisionPoints &collision)
    {
        CollisionPoints inverted = collision;
        inverted.a = collision.b;
        inverted.b = collision.a;
        inverted.normal *= -1.0f;
        return inverted;
    }

    void wakeBody(BodyObject &body)
    {
        const bool wasSleeping = body.isSleeping();
        body.wakeUp();
        if (wasSleeping && !body.isSleeping())
        {
            body.notifyWakeUp();
        }
    }

    Vector3 componentMultiply(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
    }

    Vector3 applyInverseInertiaTensor(const BodyObject &body, const Vector3 &vector)
    {
        const RigidBody *rigidBody = body.getRigidBody();
        const BoxCollider *boxCollider = dynamic_cast<const BoxCollider *>(body.getCollider());
        if (!rigidBody || !boxCollider || rigidBody->mass <= 0.0f)
        {
            return Vector3::zero();
        }

        const Transform &transform = body.getTransform();
        const Vector3 halfExtents = componentMultiply(boxCollider->halfExtents, Vector3(
                                                                                    std::abs(transform.scale.x),
                                                                                    std::abs(transform.scale.y),
                                                                                    std::abs(transform.scale.z)));
        const Vector3 fullExtents = halfExtents * 2.0f;

        const float x2 = fullExtents.x * fullExtents.x;
        const float y2 = fullExtents.y * fullExtents.y;
        const float z2 = fullExtents.z * fullExtents.z;
        const float factor = rigidBody->mass / 12.0f;
        const float inertiaX = factor * (y2 + z2);
        const float inertiaY = factor * (x2 + z2);
        const float inertiaZ = factor * (x2 + y2);

        const float inverseInertiaX = inertiaX > 0.0f ? 1.0f / inertiaX : 0.0f;
        const float inverseInertiaY = inertiaY > 0.0f ? 1.0f / inertiaY : 0.0f;
        const float inverseInertiaZ = inertiaZ > 0.0f ? 1.0f / inertiaZ : 0.0f;

        const Matrix4 rotationMatrix = transform.rotation.toMatrix();
        const Vector3 axisX = rotationMatrix.transformVector(Vector3::right()).normalized();
        const Vector3 axisY = rotationMatrix.transformVector(Vector3::up()).normalized();
        const Vector3 axisZ = rotationMatrix.transformVector(Vector3::forward()).normalized();

        return axisX * (axisX.dot(vector) * inverseInertiaX) +
               axisY * (axisY.dot(vector) * inverseInertiaY) +
               axisZ * (axisZ.dot(vector) * inverseInertiaZ);
    }

    bool isSupportingContact(const Vector3 &supportNormal)
    {
        if (supportNormal.lengthSquared() == 0.0f)
        {
            return false;
        }

        return supportNormal.normalized().dot(Vector3::up()) >= kSupportContactNormalThreshold;
    }

    bool isRigidBodyNearlyResting(const RigidBody &rigidBody)
    {
        const float sleepLinearThresholdSquared = kSleepLinearSpeedThreshold * kSleepLinearSpeedThreshold;
        const float sleepAngularThresholdSquared = kSleepAngularSpeedThreshold * kSleepAngularSpeedThreshold;
        return rigidBody.linearVelocity.lengthSquared() <= sleepLinearThresholdSquared &&
               rigidBody.angularVelocity.lengthSquared() <= sleepAngularThresholdSquared;
    }

    Vector3 midpoint(const CollisionPoints &collision)
    {
        return (collision.a + collision.b) * 0.5f;
    }

    bool canMergeContacts(const CollisionPoints &current, const CollisionPoints &previous)
    {
        if (!current.hasCollision || !previous.hasCollision)
        {
            return false;
        }

        const Vector3 currentNormal = current.normal.normalized();
        const Vector3 previousNormal = previous.normal.normalized();
        if (currentNormal.lengthSquared() == 0.0f || previousNormal.lengthSquared() == 0.0f)
        {
            return false;
        }

        if (currentNormal.dot(previousNormal) < kPersistentNormalDotThreshold)
        {
            return false;
        }

        return Vector3::distance(midpoint(current), midpoint(previous)) <= kPersistentContactDistanceThreshold;
    }

    CollisionPoints mergeContacts(const CollisionPoints &current, const CollisionPoints &previous, int persistence)
    {
        if (!canMergeContacts(current, previous))
        {
            return current;
        }

        const float previousWeight = Vector3::clamp(0.2f + static_cast<float>(std::min(persistence, 4)) * 0.15f, 0.0f, 0.8f);
        const float currentWeight = 1.0f - previousWeight;

        CollisionPoints merged;
        merged.hasCollision = true;
        merged.a = current.a * currentWeight + previous.a * previousWeight;
        merged.b = current.b * currentWeight + previous.b * previousWeight;
        merged.normal = (current.normal * currentWeight + previous.normal * previousWeight).normalized();
        merged.depth = std::max(current.depth, previous.depth * previousWeight + current.depth * currentWeight);
        return merged;
    }
}

PhysicsSystem::PhysicsSystem()
    : broadPhase(std::make_unique<SapBroadPhase>())
{
}

void PhysicsSystem::addBody(BodyObject &body)
{
    if (!body.hasRigidBody() && !body.hasCollider())
    {
        return;
    }

    if (std::find(bodies.begin(), bodies.end(), &body) != bodies.end())
    {
        return;
    }

    bodies.push_back(&body);
    refreshActiveBodies();
}

void PhysicsSystem::removeBody(const BodyObject &body)
{
    bodies.erase(std::remove(bodies.begin(), bodies.end(), &body), bodies.end());
    activeBodies.erase(std::remove(activeBodies.begin(), activeBodies.end(), &body), activeBodies.end());
}

void PhysicsSystem::clearBodies()
{
    bodies.clear();
    activeBodies.clear();
    detectedContacts.clear();
    contacts.clear();
    manifolds.clear();
    activeCollisions.clear();
}

void PhysicsSystem::setBroadPhaseCompute(BroadPhaseCompute *computeBackend)
{
    if (computeBackend)
    {
        broadPhase = std::make_unique<GpuBroadPhase>(computeBackend);
        return;
    }

    broadPhase = std::make_unique<SapBroadPhase>();
}

bool PhysicsSystem::step(float dt)
{
    if (dt <= 0.0f)
    {
        return false;
    }

    applyGlobalForces();
    integrateVelocities(dt);
    if (!hasAwakeDynamicBodies())
    {
        broadPhasePairs.clear();
        contacts.clear();
        clearAccumulators();
        return false;
    }
    broadPhasePass();
    narrowPhase();
    buildContacts();
    solveVelocityConstraints(kSolverIterations, dt);
    const bool movedFromVelocity = integratePositions(dt);
    const bool movedFromPositionSolve = solvePositionConstraints(kSolverIterations);
    updateCollisionCallbacks();
    updateSleep(dt);
    refreshActiveBodies();
    if (!hasAwakeDynamicBodies())
    {
        broadPhasePairs.clear();
        contacts.clear();
    }
    clearAccumulators();
    return movedFromVelocity || movedFromPositionSolve;
}

void PhysicsSystem::applyGlobalForces()
{
    for (BodyObject *body : activeBodies)
    {
        if (!body)
        {
            continue;
        }

        const BodyObjectDesc &config = body->getConfig();
        RigidBody *rigidBody = body->getRigidBody();
        if (!rigidBody || config.motionType == BodyMotionType::Static || config.simulateOnGpu || rigidBody->sleeping)
        {
            continue;
        }

        if (rigidBody->useGravity && rigidBody->mass > 0.0f)
        {
            rigidBody->accumulatedForce += kGravity * (rigidBody->gravityScale * rigidBody->mass);
            body->setRigidBody(*rigidBody);
        }
    }
}

void PhysicsSystem::integrateVelocities(float dt)
{
    for (BodyObject *body : activeBodies)
    {
        if (!body)
        {
            continue;
        }

        const BodyObjectDesc &config = body->getConfig();
        RigidBody *rigidBody = body->getRigidBody();
        if (!rigidBody || config.motionType == BodyMotionType::Static || config.simulateOnGpu || rigidBody->sleeping)
        {
            continue;
        }

        if (rigidBody->mass > 0.0f)
        {
            rigidBody->linearVelocity += (rigidBody->accumulatedForce * rigidBody->getInverseMass()) * dt;
        }
        rigidBody->angularVelocity += applyInverseInertiaTensor(*body, rigidBody->accumulatedTorque) * dt;

        rigidBody->linearVelocity *= std::pow(kLinearDampingPerSecond, dt * 60.0f);
        rigidBody->angularVelocity *= std::pow(kAngularDampingPerSecond, dt * 60.0f);
        body->setRigidBody(*rigidBody);
    }
}

void PhysicsSystem::broadPhasePass()
{
    broadPhasePairs = broadPhase->computePairs(buildBroadPhaseBodies());
}

void PhysicsSystem::narrowPhase()
{
    detectedContacts.clear();
    detectedContacts.reserve(broadPhasePairs.size());

    for (const BroadPhasePair &candidatePair : broadPhasePairs)
    {
        BodyObject *bodyA = candidatePair.a;
        BodyObject *bodyB = candidatePair.b;
        if (!bodyA || !bodyB)
        {
            continue;
        }

        if (isBodyPassiveForCurrentStep(*bodyA) && isBodyPassiveForCurrentStep(*bodyB))
        {
            continue;
        }

        const Collider *colliderA = bodyA->getCollider();
        const Collider *colliderB = bodyB->getCollider();
        if (!colliderA || !colliderB)
        {
            continue;
        }

        const CollisionPoints collision = colliderA->testCollision(
            *colliderB,
            bodyA->getTransform(),
            bodyB->getTransform());
        if (!collision.hasCollision)
        {
            continue;
        }

        detectedContacts.push_back(Contact{bodyA, bodyB, collision});
    }
}

void PhysicsSystem::buildContacts()
{
    std::unordered_map<BodyPair, ContactManifold, BodyPairHasher> nextManifolds;
    nextManifolds.reserve(detectedContacts.size());

    for (const Contact &detectedContact : detectedContacts)
    {
        if (!detectedContact.a || !detectedContact.b || !detectedContact.collision.hasCollision)
        {
            continue;
        }

        const BodyPair pair = detectedContact.a < detectedContact.b
                                  ? BodyPair{detectedContact.a, detectedContact.b}
                                  : BodyPair{detectedContact.b, detectedContact.a};

        ContactManifold manifold;
        manifold.a = pair.a;
        manifold.b = pair.b;
        manifold.persistence = 1;

        const Collider *colliderA = manifold.a ? manifold.a->getCollider() : nullptr;
        const Collider *colliderB = manifold.b ? manifold.b->getCollider() : nullptr;
        if (const auto *boxA = dynamic_cast<const BoxCollider *>(colliderA))
        {
            if (const auto *planeB = dynamic_cast<const PlaneCollider *>(colliderB))
            {
                manifold.points = CollisionTests::testBoxPlaneContactManifold(*boxA, manifold.a->getTransform(), *planeB, manifold.b->getTransform());
            }
        }
        else if (const auto *planeA = dynamic_cast<const PlaneCollider *>(colliderA))
        {
            if (const auto *boxB = dynamic_cast<const BoxCollider *>(colliderB))
            {
                std::vector<CollisionPoints> boxPlane = CollisionTests::testBoxPlaneContactManifold(*boxB, manifold.b->getTransform(), *planeA, manifold.a->getTransform());
                manifold.points.reserve(boxPlane.size());
                for (const CollisionPoints &point : boxPlane)
                {
                    manifold.points.push_back(invertCollision(point));
                }
            }
        }

        if (manifold.points.empty())
        {
            manifold.points.push_back(detectedContact.a < detectedContact.b
                                          ? detectedContact.collision
                                          : invertCollision(detectedContact.collision));
        }
        manifold.representative = manifold.points.front();
        for (const CollisionPoints &point : manifold.points)
        {
            if (point.depth > manifold.representative.depth)
            {
                manifold.representative = point;
            }
        }

        const auto previousIt = manifolds.find(pair);
        if (previousIt != manifolds.end())
        {
            manifold.representative = mergeContacts(manifold.representative, previousIt->second.representative, previousIt->second.persistence);
            manifold.persistence = canMergeContacts(manifold.representative, previousIt->second.representative)
                                       ? previousIt->second.persistence + 1
                                       : 1;
        }

        nextManifolds[pair] = manifold;
    }

    manifolds = std::move(nextManifolds);

    contacts.clear();
    contacts.reserve(manifolds.size());
    for (const auto &entry : manifolds)
    {
        const ContactManifold &manifold = entry.second;
        if (!manifold.a || !manifold.b)
        {
            continue;
        }

        for (const CollisionPoints &point : manifold.points)
        {
            if (!point.hasCollision)
            {
                continue;
            }

            contacts.push_back(Contact{manifold.a, manifold.b, point});
        }
    }
}

void PhysicsSystem::solveVelocityConstraint(BodyObject &bodyA, BodyObject &bodyB, const CollisionPoints &collision, float dt)
{
    (void)dt;

    const auto getInverseMass = [](BodyObject &body) -> float
    {
        const BodyObjectDesc &config = body.getConfig();
        if (config.motionType == BodyMotionType::Static)
        {
            return 0.0f;
        }

        const RigidBody *rigidBody = body.getRigidBody();
        if (!rigidBody)
        {
            return 0.0f;
        }

        return rigidBody->getInverseMass();
    };

    const float inverseMassA = getInverseMass(bodyA);
    const float inverseMassB = getInverseMass(bodyB);
    const float inverseMassSum = inverseMassA + inverseMassB;
    RigidBody *rigidBodyA = bodyA.getRigidBody();
    RigidBody *rigidBodyB = bodyB.getRigidBody();
    if (!rigidBodyA && !rigidBodyB)
    {
        return;
    }

    if (inverseMassSum <= 0.0f || collision.normal.lengthSquared() == 0.0f)
    {
        return;
    }

    const Vector3 normal = collision.normal.normalized();
    const Vector3 centerA = bodyA.getTransform().position;
    const Vector3 centerB = bodyB.getTransform().position;
    const Vector3 rA = collision.a - centerA;
    const Vector3 rB = collision.b - centerB;
    const Vector3 velocityA =
        (rigidBodyA ? rigidBodyA->linearVelocity : Vector3::zero()) +
        (rigidBodyA ? rigidBodyA->angularVelocity.cross(rA) : Vector3::zero());
    const Vector3 velocityB =
        (rigidBodyB ? rigidBodyB->linearVelocity : Vector3::zero()) +
        (rigidBodyB ? rigidBodyB->angularVelocity.cross(rB) : Vector3::zero());
    const Vector3 relativeVelocity = velocityB - velocityA;
    const float velocityAlongNormal = relativeVelocity.dot(normal);
    if (velocityAlongNormal >= 0.0f)
    {
        return;
    }

    const float restitutionA = rigidBodyA ? rigidBodyA->restitution : 0.0f;
    const float restitutionB = rigidBodyB ? rigidBodyB->restitution : 0.0f;
    float restitution = std::max(
        std::max(restitutionA, restitutionB),
        std::max(bodyA.getConfig().contactRestitution, bodyB.getConfig().contactRestitution));
    if (std::abs(velocityAlongNormal) < kRestitutionVelocityThreshold)
    {
        restitution = 0.0f;
    }
    const Vector3 angularFactorA = applyInverseInertiaTensor(bodyA, rA.cross(normal)).cross(rA);
    const Vector3 angularFactorB = applyInverseInertiaTensor(bodyB, rB.cross(normal)).cross(rB);
    const float angularDenominator = (angularFactorA + angularFactorB).dot(normal);
    const float impulseMagnitude = -(1.0f + restitution) * velocityAlongNormal / (inverseMassSum + angularDenominator);
    const Vector3 impulse = normal * impulseMagnitude;
    const bool isRestingContact = std::abs(velocityAlongNormal) < kRestingContactVelocityThreshold;

    if (rigidBodyA && inverseMassA > 0.0f)
    {
        rigidBodyA->linearVelocity -= impulse * inverseMassA;
        rigidBodyA->angularVelocity -= applyInverseInertiaTensor(bodyA, rA.cross(impulse));
        if (isRestingContact || std::abs(rigidBodyA->linearVelocity.dot(normal)) < kSleepLinearSpeedThreshold)
        {
            rigidBodyA->linearVelocity -= normal * rigidBodyA->linearVelocity.dot(normal);
        }
        bodyA.setRigidBody(*rigidBodyA);
    }

    if (rigidBodyB && inverseMassB > 0.0f)
    {
        rigidBodyB->linearVelocity += impulse * inverseMassB;
        rigidBodyB->angularVelocity += applyInverseInertiaTensor(bodyB, rB.cross(impulse));
        if (isRestingContact || std::abs(rigidBodyB->linearVelocity.dot(normal)) < kSleepLinearSpeedThreshold)
        {
            rigidBodyB->linearVelocity -= normal * rigidBodyB->linearVelocity.dot(normal);
        }
        bodyB.setRigidBody(*rigidBodyB);
    }

    const Vector3 updatedVelocityA =
        (rigidBodyA ? rigidBodyA->linearVelocity : Vector3::zero()) +
        (rigidBodyA ? rigidBodyA->angularVelocity.cross(rA) : Vector3::zero());
    const Vector3 updatedVelocityB =
        (rigidBodyB ? rigidBodyB->linearVelocity : Vector3::zero()) +
        (rigidBodyB ? rigidBodyB->angularVelocity.cross(rB) : Vector3::zero());
    const Vector3 updatedRelativeVelocity = updatedVelocityB - updatedVelocityA;
    Vector3 tangent = updatedRelativeVelocity - normal * updatedRelativeVelocity.dot(normal);
    if (tangent.lengthSquared() > 0.0f)
    {
        tangent = tangent.normalized();
        const Vector3 tangentAngularFactorA = applyInverseInertiaTensor(bodyA, rA.cross(tangent)).cross(rA);
        const Vector3 tangentAngularFactorB = applyInverseInertiaTensor(bodyB, rB.cross(tangent)).cross(rB);
        const float tangentDenominator = inverseMassSum + (tangentAngularFactorA + tangentAngularFactorB).dot(tangent);
        if (tangentDenominator > 0.0f)
        {
            const float tangentSpeed = updatedRelativeVelocity.dot(tangent);
            float tangentImpulseMagnitude = -tangentSpeed / tangentDenominator;
            const float friction = std::sqrt(std::max(0.0f, bodyA.getConfig().contactFriction * bodyB.getConfig().contactFriction));
            const float maxFrictionImpulse = impulseMagnitude * friction;
            tangentImpulseMagnitude = Vector3::clamp(tangentImpulseMagnitude, -maxFrictionImpulse, maxFrictionImpulse);
            const Vector3 tangentImpulse = tangent * tangentImpulseMagnitude;

            if (rigidBodyA && inverseMassA > 0.0f)
            {
                rigidBodyA->linearVelocity -= tangentImpulse * inverseMassA;
                rigidBodyA->angularVelocity -= applyInverseInertiaTensor(bodyA, rA.cross(tangentImpulse));
                const float tangentSpeedA = rigidBodyA->linearVelocity.dot(tangent);
                if (isRestingContact && std::abs(tangentSpeedA) < kRestingTangentVelocityThreshold)
                {
                    rigidBodyA->linearVelocity -= tangent * tangentSpeedA;
                }
                if (isRestingContact && rigidBodyA->angularVelocity.lengthSquared() < kRestingAngularSpeedThreshold * kRestingAngularSpeedThreshold)
                {
                    rigidBodyA->angularVelocity = Vector3::zero();
                }
                bodyA.setRigidBody(*rigidBodyA);
            }

            if (rigidBodyB && inverseMassB > 0.0f)
            {
                rigidBodyB->linearVelocity += tangentImpulse * inverseMassB;
                rigidBodyB->angularVelocity += applyInverseInertiaTensor(bodyB, rB.cross(tangentImpulse));
                const float tangentSpeedB = rigidBodyB->linearVelocity.dot(tangent);
                if (isRestingContact && std::abs(tangentSpeedB) < kRestingTangentVelocityThreshold)
                {
                    rigidBodyB->linearVelocity -= tangent * tangentSpeedB;
                }
                if (isRestingContact && rigidBodyB->angularVelocity.lengthSquared() < kRestingAngularSpeedThreshold * kRestingAngularSpeedThreshold)
                {
                    rigidBodyB->angularVelocity = Vector3::zero();
                }
                bodyB.setRigidBody(*rigidBodyB);
            }
        }
    }
    else if (isRestingContact)
    {
        if (rigidBodyA && inverseMassA > 0.0f && rigidBodyA->angularVelocity.lengthSquared() < kRestingAngularSpeedThreshold * kRestingAngularSpeedThreshold)
        {
            rigidBodyA->angularVelocity = Vector3::zero();
            bodyA.setRigidBody(*rigidBodyA);
        }

        if (rigidBodyB && inverseMassB > 0.0f && rigidBodyB->angularVelocity.lengthSquared() < kRestingAngularSpeedThreshold * kRestingAngularSpeedThreshold)
        {
            rigidBodyB->angularVelocity = Vector3::zero();
            bodyB.setRigidBody(*rigidBodyB);
        }
    }
}

bool PhysicsSystem::integratePositions(float dt)
{
    bool anyBodyMoved = false;
    for (BodyObject *body : activeBodies)
    {
        if (!body)
        {
            continue;
        }

        const BodyObjectDesc &config = body->getConfig();
        RigidBody *rigidBody = body->getRigidBody();
        if (!rigidBody || config.motionType == BodyMotionType::Static || config.simulateOnGpu || rigidBody->sleeping)
        {
            continue;
        }

        const float linearSpeedSquared = rigidBody->linearVelocity.lengthSquared();
        const float angularSpeedSquared = rigidBody->angularVelocity.lengthSquared();
        if (linearSpeedSquared == 0.0f && angularSpeedSquared == 0.0f)
        {
            continue;
        }

        Transform transform = body->getTransform();
        transform.position += rigidBody->linearVelocity * dt;
        const Quaternion deltaRotation = Quaternion::fromEulerXYZ(rigidBody->angularVelocity * dt);
        transform.rotation = (deltaRotation * transform.rotation).normalized();
        body->setTransform(transform);
        anyBodyMoved = true;
    }

    return anyBodyMoved;
}

bool PhysicsSystem::solvePositionConstraint(BodyObject &bodyA, BodyObject &bodyB, const CollisionPoints &collision)
{
    const auto getInverseMass = [](BodyObject &body) -> float
    {
        const BodyObjectDesc &config = body.getConfig();
        if (config.motionType == BodyMotionType::Static)
        {
            return 0.0f;
        }

        const RigidBody *rigidBody = body.getRigidBody();
        if (!rigidBody)
        {
            return 0.0f;
        }

        return rigidBody->getInverseMass();
    };

    const float inverseMassA = getInverseMass(bodyA);
    const float inverseMassB = getInverseMass(bodyB);
    const float inverseMassSum = inverseMassA + inverseMassB;
    if (inverseMassSum <= 0.0f || collision.depth <= 0.0f || collision.normal.lengthSquared() == 0.0f)
    {
        return false;
    }

    const float correctionDepth = std::max(collision.depth - kPenetrationSlop, 0.0f);
    if (correctionDepth <= 0.0f)
    {
        return false;
    }

    const Vector3 correction = collision.normal.normalized() * ((correctionDepth * kPenetrationCorrectionPercent) / inverseMassSum);
    bool moved = false;

    if (inverseMassA > 0.0f)
    {
        Transform transformA = bodyA.getTransform();
        transformA.position -= correction * inverseMassA;
        bodyA.setTransform(transformA);
        moved = true;
    }

    if (inverseMassB > 0.0f)
    {
        Transform transformB = bodyB.getTransform();
        transformB.position += correction * inverseMassB;
        bodyB.setTransform(transformB);
        moved = true;
    }

    return moved;
}

bool PhysicsSystem::solvePositionConstraints(int iterations)
{
    bool anyBodyMoved = false;
    for (int iteration = 0; iteration < iterations; ++iteration)
    {
        narrowPhase();
        buildContacts();

        for (const Contact &contact : contacts)
        {
            if (!contact.a || !contact.b)
            {
                continue;
            }

            if (isBodyPassiveForCurrentStep(*contact.a) && isBodyPassiveForCurrentStep(*contact.b))
            {
                continue;
            }

            anyBodyMoved = solvePositionConstraint(*contact.a, *contact.b, contact.collision) || anyBodyMoved;
        }
    }

    narrowPhase();
    buildContacts();
    return anyBodyMoved;
}

void PhysicsSystem::solveVelocityConstraints(int iterations, float dt)
{
    const float iterationDt = iterations > 0 ? dt / static_cast<float>(iterations) : 0.0f;
    for (int iteration = 0; iteration < iterations; ++iteration)
    {
        for (const Contact &contact : contacts)
        {
            if (!contact.a || !contact.b)
            {
                continue;
            }

            if (isBodyPassiveForCurrentStep(*contact.a) && isBodyPassiveForCurrentStep(*contact.b))
            {
                continue;
            }

            solveVelocityConstraint(*contact.a, *contact.b, contact.collision, iterationDt);
        }
    }
}

void PhysicsSystem::updateCollisionCallbacks()
{
    std::unordered_set<BodyPair, BodyPairHasher> currentCollisions;
    const auto makeBodyPair = [](BodyObject &bodyA, BodyObject &bodyB)
    {
        return &bodyA < &bodyB ? BodyPair{&bodyA, &bodyB} : BodyPair{&bodyB, &bodyA};
    };

    for (const auto &entry : manifolds)
    {
        const ContactManifold &manifold = entry.second;
        if (!manifold.a || !manifold.b || !manifold.representative.hasCollision)
        {
            continue;
        }

        const Vector3 normal = manifold.representative.normal.lengthSquared() > 0.0f
                                   ? manifold.representative.normal.normalized()
                                   : Vector3::zero();
        const Vector3 velocityA = manifold.a->getRigidBody() ? manifold.a->getRigidBody()->linearVelocity : Vector3::zero();
        const Vector3 velocityB = manifold.b->getRigidBody() ? manifold.b->getRigidBody()->linearVelocity : Vector3::zero();
        const float impactSpeedAlongNormal = std::abs((velocityB - velocityA).dot(normal));
        const BodyPair pair = makeBodyPair(*manifold.a, *manifold.b);
        currentCollisions.insert(pair);

        if (impactSpeedAlongNormal > kWakeVelocityThreshold)
        {
            wakeBody(*manifold.a);
            wakeBody(*manifold.b);
        }

        if (activeCollisions.find(pair) == activeCollisions.end())
        {
            manifold.a->notifyCollisionEnter(*manifold.b, manifold.representative);
            manifold.b->notifyCollisionEnter(*manifold.a, invertCollision(manifold.representative));
        }

        manifold.a->notifyCollisionStay(*manifold.b, manifold.representative);
        manifold.b->notifyCollisionStay(*manifold.a, invertCollision(manifold.representative));
    }

    for (const BodyPair &pair : activeCollisions)
    {
        if (currentCollisions.find(pair) != currentCollisions.end() || !pair.a || !pair.b)
        {
            continue;
        }

        const CollisionPoints emptyCollision{};
        pair.a->notifyCollisionExit(*pair.b, emptyCollision);
        pair.b->notifyCollisionExit(*pair.a, emptyCollision);
    }

    activeCollisions = std::move(currentCollisions);
}

void PhysicsSystem::updateSleep(float dt)
{
    const float sleepLinearThresholdSquared = kSleepLinearSpeedThreshold * kSleepLinearSpeedThreshold;
    const float sleepAngularThresholdSquared = kSleepAngularSpeedThreshold * kSleepAngularSpeedThreshold;
    const float restingTangentThresholdSquared = kRestingTangentVelocityThreshold * kRestingTangentVelocityThreshold;
    const float restingAngularThresholdSquared = kRestingAngularSpeedThreshold * kRestingAngularSpeedThreshold;

    for (BodyObject *body : activeBodies)
    {
        if (!body)
        {
            continue;
        }

        const BodyObjectDesc &config = body->getConfig();
        RigidBody *rigidBody = body->getRigidBody();
        if (!rigidBody || config.motionType == BodyMotionType::Static || config.simulateOnGpu)
        {
            continue;
        }

        Vector3 supportNormal = Vector3::zero();
        if (getStableSupportNormal(*body, supportNormal))
        {
            const Vector3 normalVelocity = supportNormal * rigidBody->linearVelocity.dot(supportNormal);
            const Vector3 tangentialVelocity = rigidBody->linearVelocity - normalVelocity;
            if (tangentialVelocity.lengthSquared() < restingTangentThresholdSquared)
            {
                rigidBody->linearVelocity = normalVelocity;
            }

            if (rigidBody->angularVelocity.lengthSquared() < restingAngularThresholdSquared)
            {
                rigidBody->angularVelocity = Vector3::zero();
            }
        }

        if (rigidBody->linearVelocity.lengthSquared() < sleepLinearThresholdSquared)
        {
            rigidBody->linearVelocity = Vector3::zero();
        }
        if (rigidBody->angularVelocity.lengthSquared() < sleepAngularThresholdSquared)
        {
            rigidBody->angularVelocity = Vector3::zero();
        }

        const float linearSpeedSquared = rigidBody->linearVelocity.lengthSquared();
        const float angularSpeedSquared = rigidBody->angularVelocity.lengthSquared();
        if (rigidBody->canSleep &&
            linearSpeedSquared <= sleepLinearThresholdSquared &&
            angularSpeedSquared <= sleepAngularThresholdSquared &&
            canBodySleepOnCurrentContacts(*body))
        {
            rigidBody->sleepTime += dt;
            if (rigidBody->sleepTime >= kSleepDelaySeconds)
            {
                rigidBody->sleeping = true;
                rigidBody->sleepTime = kSleepDelaySeconds;
                rigidBody->linearVelocity = Vector3::zero();
                rigidBody->angularVelocity = Vector3::zero();
                body->setRigidBody(*rigidBody);
                body->notifySleep();
                continue;
            }

            body->setRigidBody(*rigidBody);
            continue;
        }

        if (rigidBody->sleepTime > 0.0f || rigidBody->sleeping)
        {
            const bool wasSleeping = rigidBody->sleeping;
            rigidBody->sleepTime = 0.0f;
            rigidBody->sleeping = false;
            body->setRigidBody(*rigidBody);
            if (wasSleeping)
            {
                body->notifyWakeUp();
            }
            continue;
        }

        body->setRigidBody(*rigidBody);
    }
}

void PhysicsSystem::clearAccumulators()
{
    for (BodyObject *body : activeBodies)
    {
        if (!body)
        {
            continue;
        }

        RigidBody *rigidBody = body->getRigidBody();
        if (!rigidBody)
        {
            continue;
        }

        rigidBody->accumulatedForce = Vector3::zero();
        rigidBody->accumulatedTorque = Vector3::zero();
        body->setRigidBody(*rigidBody);
    }
}

void PhysicsSystem::refreshActiveBodies()
{
    activeBodies.clear();
    activeBodies.reserve(bodies.size());
    for (BodyObject *body : bodies)
    {
        if (!body)
        {
            continue;
        }

        const BodyObjectDesc &config = body->getConfig();
        const RigidBody *rigidBody = body->getRigidBody();
        if (!rigidBody || config.motionType == BodyMotionType::Static || config.simulateOnGpu)
        {
            continue;
        }

        if (!rigidBody->sleeping)
        {
            activeBodies.push_back(body);
        }
    }
}

std::vector<BodyObject *> PhysicsSystem::buildBroadPhaseBodies() const
{
    std::vector<BodyObject *> broadPhaseBodies;
    broadPhaseBodies.reserve(bodies.size());
    for (BodyObject *body : bodies)
    {
        if (body && body->hasCollider())
        {
            broadPhaseBodies.push_back(body);
        }
    }

    return broadPhaseBodies;
}

bool PhysicsSystem::hasAwakeDynamicBodies() const
{
    return !activeBodies.empty();
}

bool PhysicsSystem::isBodyPassiveForCurrentStep(const BodyObject &body) const
{
    const BodyObjectDesc &config = body.getConfig();
    if (config.motionType == BodyMotionType::Static || config.simulateOnGpu)
    {
        return true;
    }

    const RigidBody *rigidBody = body.getRigidBody();
    if (!rigidBody)
    {
        return true;
    }

    return rigidBody->sleeping;
}

bool PhysicsSystem::canBodySleepOnCurrentContacts(const BodyObject &body) const
{
    const RigidBody *rigidBody = body.getRigidBody();
    if (!rigidBody)
    {
        return false;
    }

    bool hasAnyContact = false;
    Vector3 supportNormal = Vector3::zero();
    const bool hasStableSupport = getStableSupportNormal(body, supportNormal);
    for (const Contact &contact : contacts)
    {
        if (contact.a == &body || contact.b == &body)
        {
            hasAnyContact = true;
            break;
        }
    }

    if (rigidBody->useGravity)
    {
        return hasStableSupport;
    }

    return !hasAnyContact || hasStableSupport;
}

bool PhysicsSystem::getStableSupportNormal(const BodyObject &body, Vector3 &supportNormal) const
{
    supportNormal = Vector3::zero();
    for (const Contact &contact : contacts)
    {
        const BodyObject *other = nullptr;
        Vector3 candidateNormal = Vector3::zero();
        if (contact.a == &body)
        {
            other = contact.b;
            candidateNormal = contact.collision.normal * -1.0f;
        }
        else if (contact.b == &body)
        {
            other = contact.a;
            candidateNormal = contact.collision.normal;
        }
        else
        {
            continue;
        }

        if (!other)
        {
            continue;
        }

        const BodyObjectDesc &otherConfig = other->getConfig();
        const RigidBody *otherRigidBody = other->getRigidBody();
        const bool otherIsAwakeDynamic =
            otherConfig.motionType != BodyMotionType::Static &&
            otherRigidBody &&
            !otherRigidBody->sleeping;
        const bool otherCanActAsStableSupport =
            otherConfig.motionType == BodyMotionType::Static ||
            (otherRigidBody && (otherRigidBody->sleeping || isRigidBodyNearlyResting(*otherRigidBody)));
        if (otherIsAwakeDynamic && !otherCanActAsStableSupport)
        {
            return false;
        }

        if (!isSupportingContact(candidateNormal))
        {
            continue;
        }

        supportNormal = candidateNormal.normalized();
        return true;
    }

    return false;
}
