#include "engine/physics/PhysicsSystem.h"

#include <algorithm>

#include "engine/physics/GpuBroadPhase.h"
#include "engine/physics/SapBroadPhase.h"
#include "engine/scene/objects/BodyObject.h"

namespace
{
constexpr float kSleepLinearSpeedThreshold = 0.01f;
constexpr float kSleepAngularSpeedThreshold = 0.01f;
constexpr float kSleepDelaySeconds = 0.35f;

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
    body.wakeUp();
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
}

void PhysicsSystem::removeBody(const BodyObject &body)
{
    bodies.erase(std::remove(bodies.begin(), bodies.end(), &body), bodies.end());
}

void PhysicsSystem::clearBodies()
{
    bodies.clear();
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
    bool anyBodyMoved = false;
    for (BodyObject *body : bodies)
    {
        if (!body)
        {
            continue;
        }

        anyBodyMoved = integrateBody(*body, dt) || anyBodyMoved;
    }

    resolveCollisions(dt);
    return anyBodyMoved;
}

bool PhysicsSystem::integrateBody(BodyObject &body, float dt)
{
    if (dt <= 0.0f)
    {
        return false;
    }

    const BodyObjectDesc &config = body.getConfig();
    RigidBody *rigidBody = body.getRigidBody();
    if (!rigidBody || config.motionType == BodyMotionType::Static || config.simulateOnGpu)
    {
        return false;
    }

    RigidBody physicsState = *rigidBody;
    if (physicsState.sleeping)
    {
        return false;
    }

    const float linearSpeedSquared = physicsState.linearVelocity.lengthSquared();
    const float angularSpeedSquared = physicsState.angularVelocity.lengthSquared();
    const float sleepLinearThresholdSquared = kSleepLinearSpeedThreshold * kSleepLinearSpeedThreshold;
    const float sleepAngularThresholdSquared = kSleepAngularSpeedThreshold * kSleepAngularSpeedThreshold;

    if (physicsState.canSleep &&
        linearSpeedSquared <= sleepLinearThresholdSquared &&
        angularSpeedSquared <= sleepAngularThresholdSquared)
    {
        physicsState.sleepTime += dt;
        if (physicsState.sleepTime >= kSleepDelaySeconds)
        {
            physicsState.sleeping = true;
            physicsState.sleepTime = kSleepDelaySeconds;
            physicsState.linearVelocity = Vector3::zero();
            physicsState.angularVelocity = Vector3::zero();
        }
        body.setRigidBody(physicsState);
        return false;
    }

    if (physicsState.sleepTime > 0.0f || physicsState.sleeping)
    {
        physicsState.sleepTime = 0.0f;
        physicsState.sleeping = false;
        body.setRigidBody(physicsState);
    }

    if (linearSpeedSquared == 0.0f && angularSpeedSquared == 0.0f)
    {
        return false;
    }

    Transform transform = body.getTransform();
    transform.position += physicsState.linearVelocity * dt;
    const Quaternion deltaRotation = Quaternion::fromEulerXYZ(physicsState.angularVelocity * dt);
    transform.rotation = (deltaRotation * transform.rotation).normalized();
    body.setTransform(transform);
    body.setRigidBody(physicsState);
    return true;
}

void PhysicsSystem::solveCollision(BodyObject &bodyA, BodyObject &bodyB, const CollisionPoints &collision, float dt)
{
    (void)collision;
    (void)dt;

    const auto getInverseMass = [](BodyObject &body) -> float {
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
    if (inverseMassSum > 0.0f && collision.depth > 0.0f && collision.normal.lengthSquared() > 0.0f)
    {
        const Vector3 correction = collision.normal.normalized() * (collision.depth / inverseMassSum);

        if (inverseMassA > 0.0f)
        {
            Transform transformA = bodyA.getTransform();
            transformA.position -= correction * inverseMassA;
            bodyA.setTransform(transformA);
        }

        if (inverseMassB > 0.0f)
        {
            Transform transformB = bodyB.getTransform();
            transformB.position += correction * inverseMassB;
            bodyB.setTransform(transformB);
        }
    }

    if (RigidBody *rigidBodyA = bodyA.getRigidBody())
    {
        rigidBodyA->linearVelocity *= -1.0f;
        bodyA.setRigidBody(*rigidBodyA);
    }

    if (RigidBody *rigidBodyB = bodyB.getRigidBody())
    {
        rigidBodyB->linearVelocity *= -1.0f;
        bodyB.setRigidBody(*rigidBodyB);
    }
}

void PhysicsSystem::resolveCollisions(float dt)
{
    std::unordered_set<BodyPair, BodyPairHasher> currentCollisions;
    const auto makeBodyPair = [](BodyObject &bodyA, BodyObject &bodyB) {
        return &bodyA < &bodyB
            ? BodyPair{&bodyA, &bodyB}
            : BodyPair{&bodyB, &bodyA};
    };
    const std::vector<BroadPhasePair> candidatePairs = broadPhase->computePairs(bodies);
    for (const BroadPhasePair &candidatePair : candidatePairs)
    {
        BodyObject *bodyA = candidatePair.a;
        BodyObject *bodyB = candidatePair.b;
        if (!bodyA || !bodyB)
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

        solveCollision(*bodyA, *bodyB, collision, dt);

        const BodyPair pair = makeBodyPair(*bodyA, *bodyB);
        currentCollisions.insert(pair);

        wakeBody(*bodyA);
        wakeBody(*bodyB);

        if (activeCollisions.find(pair) == activeCollisions.end())
        {
            bodyA->notifyCollisionEnter(*bodyB, collision);
            bodyB->notifyCollisionEnter(*bodyA, invertCollision(collision));
        }

        bodyA->notifyCollisionStay(*bodyB, collision);
        bodyB->notifyCollisionStay(*bodyA, invertCollision(collision));
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
