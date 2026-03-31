#include "engine/physics/PhysicsSystem.h"

#include <algorithm>

#include "engine/physics/BoxCollider.h"
#include "engine/physics/GpuBroadPhase.h"
#include "engine/physics/SapBroadPhase.h"
#include "engine/scene/objects/BodyObject.h"

namespace
{
constexpr float kSleepLinearSpeedThreshold = 0.01f;
constexpr float kSleepAngularSpeedThreshold = 0.01f;
constexpr float kSleepDelaySeconds = 0.35f;
constexpr float kPenetrationSlop = 0.05f;
constexpr float kPenetrationCorrectionPercent = 0.35f;
constexpr float kRestitutionVelocityThreshold = 2.0f;
constexpr float kWakeVelocityThreshold = 0.35f;
constexpr float kRestingContactVelocityThreshold = 0.25f;
constexpr float kLinearDampingPerSecond = 0.985f;
constexpr float kAngularDampingPerSecond = 0.98f;
const Vector3 kGravity(0.0f, -9.81f, 0.0f);

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

    if (physicsState.useGravity && physicsState.mass > 0.0f)
    {
        physicsState.linearVelocity += kGravity * (physicsState.gravityScale * dt);
    }

    physicsState.linearVelocity *= std::pow(kLinearDampingPerSecond, dt * 60.0f);
    physicsState.angularVelocity *= std::pow(kAngularDampingPerSecond, dt * 60.0f);

    if (physicsState.linearVelocity.lengthSquared() < kSleepLinearSpeedThreshold * kSleepLinearSpeedThreshold)
    {
        physicsState.linearVelocity = Vector3::zero();
    }
    if (physicsState.angularVelocity.lengthSquared() < kSleepAngularSpeedThreshold * kSleepAngularSpeedThreshold)
    {
        physicsState.angularVelocity = Vector3::zero();
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
            body.setRigidBody(physicsState);
            body.notifySleep();
            return false;
        }
        body.setRigidBody(physicsState);
        return false;
    }

    if (physicsState.sleepTime > 0.0f || physicsState.sleeping)
    {
        const bool wasSleeping = physicsState.sleeping;
        physicsState.sleepTime = 0.0f;
        physicsState.sleeping = false;
        body.setRigidBody(physicsState);
        if (wasSleeping)
        {
            body.notifyWakeUp();
        }
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
        const float correctionDepth = std::max(collision.depth - kPenetrationSlop, 0.0f);
        const Vector3 correction = collision.normal.normalized() * ((correctionDepth * kPenetrationCorrectionPercent) / inverseMassSum);

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
                bodyA.setRigidBody(*rigidBodyA);
            }

            if (rigidBodyB && inverseMassB > 0.0f)
            {
                rigidBodyB->linearVelocity += tangentImpulse * inverseMassB;
                rigidBodyB->angularVelocity += applyInverseInertiaTensor(bodyB, rB.cross(tangentImpulse));
                bodyB.setRigidBody(*rigidBodyB);
            }
        }
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

        const Vector3 normal = collision.normal.lengthSquared() > 0.0f ? collision.normal.normalized() : Vector3::zero();
        const Vector3 velocityA = bodyA->getRigidBody() ? bodyA->getRigidBody()->linearVelocity : Vector3::zero();
        const Vector3 velocityB = bodyB->getRigidBody() ? bodyB->getRigidBody()->linearVelocity : Vector3::zero();
        const float impactSpeedAlongNormal = std::abs((velocityB - velocityA).dot(normal));

        const BodyPair pair = makeBodyPair(*bodyA, *bodyB);
        currentCollisions.insert(pair);

        if (impactSpeedAlongNormal > kWakeVelocityThreshold)
        {
            wakeBody(*bodyA);
            wakeBody(*bodyB);
        }

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
