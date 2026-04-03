#include "engine/physics/PhysicsSystem.h"

#include <algorithm>

#include "engine/math/Quaternion.h"
#include "engine/physics/CollisionTests.h"
#include "engine/physics/SapBroadPhase.h"
#include "engine/scene/objects/BodyObject.h"

namespace
{
    constexpr float kLinearDampingPerStep = 0.9995f;
    constexpr float kAngularDampingPerStep = 0.9995f;
    constexpr float kVelocityStopEpsilonSq = 1e-10f;

    CollisionPoints invertCollision(const CollisionPoints &collision)
    {
        CollisionPoints inverted = collision;
        inverted.a = collision.b;
        inverted.b = collision.a;
        inverted.normal *= -1.0f;
        return inverted;
    }

    bool shouldIncludeInBroadPhase(const BodyObject &body)
    {
        if (!body.hasCollider())
        {
            return false;
        }

        const BodyObjectDesc &config = body.getConfig();
        if (config.motionType == BodyMotionType::Static || config.motionType == BodyMotionType::Kinematic)
        {
            return true;
        }

        return !body.isSleeping();
    }
} // namespace

PhysicsSystem::PhysicsSystem()
    : broadPhase(std::make_unique<SapBroadPhase>())
{
}

void PhysicsSystem::addBody(BodyObject &body)
{
    if (!body.hasCollider())
    {
        return;
    }

    if (std::find(bodies.begin(), bodies.end(), &body) != bodies.end())
    {
        return;
    }

    bodies.push_back(&body);
    bodiesDirty = true;
}

void PhysicsSystem::removeBody(const BodyObject &body)
{
    bodies.erase(std::remove(bodies.begin(), bodies.end(), &body), bodies.end());
    bodiesDirty = true;
}

void PhysicsSystem::clearBodies()
{
    bodies.clear();
    broadPhaseBodies.clear();
    broadPhasePairs.clear();
    detectedContacts.clear();
    activeCollisions.clear();
    bodiesDirty = true;
}

bool PhysicsSystem::step(float dt)
{
    if (dt <= 0.0f)
    {
        return false;
    }

    const bool moved = integrateBodies(dt);
    if (!moved && !bodiesDirty)
    {
        return false;
    }

    broadPhasePass();
    narrowPhase();
    updateCollisionCallbacks();
    bodiesDirty = false;

    return moved || !detectedContacts.empty();
}

bool PhysicsSystem::integrateBodies(float dt)
{
    bool anyBodyMoved = false;
    for (BodyObject *body : bodies)
    {
        if (!body)
        {
            continue;
        }

        const BodyObjectDesc &config = body->getConfig();
        if (config.motionType != BodyMotionType::Dynamic && config.motionType != BodyMotionType::Kinematic)
        {
            continue;
        }

        if (config.motionType == BodyMotionType::Dynamic && body->isSleeping())
        {
            continue;
        }

        Vector3 linearVelocity = body->getLinearVelocity();
        Vector3 angularVelocity = body->getAngularVelocity();

        const bool hasLinearMotion = linearVelocity.lengthSquared() > 0.0f;
        const bool hasAngularMotion = angularVelocity.lengthSquared() > 0.0f;
        if (!hasLinearMotion && !hasAngularMotion)
        {
            continue;
        }

        Transform transform = body->getTransform();
        if (hasLinearMotion)
        {
            transform.position += linearVelocity * dt;
        }
        if (hasAngularMotion)
        {
            const Quaternion deltaRotation = Quaternion::fromEulerXYZ(angularVelocity * dt);
            transform.rotation = (deltaRotation * transform.rotation).normalized();
        }

        if (config.motionType == BodyMotionType::Dynamic)
        {
            // damping
            linearVelocity *= kLinearDampingPerStep;
            angularVelocity *= kAngularDampingPerStep;
            if (linearVelocity.lengthSquared() < kVelocityStopEpsilonSq)
            {
                linearVelocity = Vector3::zero();
            }
            if (angularVelocity.lengthSquared() < kVelocityStopEpsilonSq)
            {
                angularVelocity = Vector3::zero();
            }
            body->setVelocities(linearVelocity, angularVelocity);
            if (linearVelocity.lengthSquared() <= 0.0f && angularVelocity.lengthSquared() <= 0.0f)
            {
                body->setSleeping(true);
            }
        }

        body->setTransform(transform);
        anyBodyMoved = true;
    }

    return anyBodyMoved;
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

void PhysicsSystem::updateCollisionCallbacks()
{
    std::unordered_set<BodyPair, BodyPairHasher> currentCollisions;
    const auto makeBodyPair = [](BodyObject &bodyA, BodyObject &bodyB)
    {
        return &bodyA < &bodyB ? BodyPair{&bodyA, &bodyB} : BodyPair{&bodyB, &bodyA};
    };

    std::unordered_map<BodyPair, CollisionPoints, BodyPairHasher> pairContacts;
    pairContacts.reserve(detectedContacts.size());

    for (const Contact &detectedContact : detectedContacts)
    {
        if (!detectedContact.a || !detectedContact.b || !detectedContact.collision.hasCollision)
        {
            continue;
        }

        const BodyPair pair = makeBodyPair(*detectedContact.a, *detectedContact.b);
        CollisionPoints collisionForPair = detectedContact.a < detectedContact.b
                                               ? detectedContact.collision
                                               : invertCollision(detectedContact.collision);

        const auto existing = pairContacts.find(pair);
        if (existing == pairContacts.end() || collisionForPair.depth > existing->second.depth)
        {
            pairContacts[pair] = collisionForPair;
        }
    }

    for (const auto &entry : pairContacts)
    {
        const BodyPair &pair = entry.first;
        const CollisionPoints &collision = entry.second;
        if (!pair.a || !pair.b || !collision.hasCollision)
        {
            continue;
        }

        currentCollisions.insert(pair);

        if (activeCollisions.find(pair) == activeCollisions.end())
        {
            pair.a->notifyCollisionEnter(*pair.b, collision);
            pair.b->notifyCollisionEnter(*pair.a, invertCollision(collision));
        }

        // pair.a->notifyCollisionStay(*pair.b, collision);
        // pair.b->notifyCollisionStay(*pair.a, invertCollision(collision));
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

const std::vector<BodyObject *> &PhysicsSystem::buildBroadPhaseBodies()
{
    broadPhaseBodies.clear();
    broadPhaseBodies.reserve(bodies.size());
    for (BodyObject *body : bodies)
    {
        if (body && shouldIncludeInBroadPhase(*body))
        {
            broadPhaseBodies.push_back(body);
        }
    }

    return broadPhaseBodies;
}
