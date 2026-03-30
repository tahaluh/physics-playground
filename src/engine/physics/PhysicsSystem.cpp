#include "engine/physics/PhysicsSystem.h"

#include <algorithm>
#include <vector>

#include "engine/physics/Collider.h"
#include "engine/scene/objects/BodyObject.h"

namespace
{
CollisionPoints invertCollision(const CollisionPoints &collision)
{
    CollisionPoints inverted = collision;
    inverted.a = collision.b;
    inverted.b = collision.a;
    inverted.normal *= -1.0f;
    return inverted;
}
}

void PhysicsSystem::addBody(BodyObject &body)
{
    if (body.getConfig().motionType == BodyMotionType::Static)
    {
        return;
    }

    if (std::find(bodies.begin(), bodies.end(), &body) != bodies.end())
    {
        return;
    }

    bodies.push_back(&body);
    sweepAndPruneOrder.push_back(&body);
}

void PhysicsSystem::removeBody(const BodyObject &body)
{
    bodies.erase(std::remove(bodies.begin(), bodies.end(), &body), bodies.end());
    sweepAndPruneOrder.erase(std::remove(sweepAndPruneOrder.begin(), sweepAndPruneOrder.end(), &body), sweepAndPruneOrder.end());
}

void PhysicsSystem::clearBodies()
{
    bodies.clear();
    sweepAndPruneOrder.clear();
    activeCollisions.clear();
}

bool PhysicsSystem::step(float dt)
{
    bool anyBodyMoved = false;
    if (sweepAndPruneOrder.size() != bodies.size())
    {
        sweepAndPruneOrder = bodies;
    }

    for (BodyObject *body : sweepAndPruneOrder)
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
    if (config.motionType == BodyMotionType::Static || config.simulateOnGpu)
    {
        return false;
    }

    const BodyPhysicsState &physicsState = body.getPhysicsState();
    if (physicsState.linearVelocity.lengthSquared() == 0.0f &&
        physicsState.angularVelocity.lengthSquared() == 0.0f)
    {
        return false;
    }

    Transform transform = body.getTransform();
    transform.position += physicsState.linearVelocity * dt;
    const Quaternion deltaRotation = Quaternion::fromEulerXYZ(physicsState.angularVelocity * dt);
    transform.rotation = (deltaRotation * transform.rotation).normalized();
    body.setTransform(transform);
    return true;
}

void PhysicsSystem::resolveCollisions(float dt)
{
    (void)dt;

    struct SweepEntry
    {
        BodyObject *body = nullptr;
        const Collider *collider = nullptr;
        Collider::BroadPhaseBounds bounds;
    };

    std::unordered_set<BodyPair, BodyPairHasher> currentCollisions;
    const auto makeBodyPair = [](BodyObject &bodyA, BodyObject &bodyB) {
        return &bodyA < &bodyB
            ? BodyPair{&bodyA, &bodyB}
            : BodyPair{&bodyB, &bodyA};
    };
    const auto overlapsOnYZ = [](const Collider::BroadPhaseBounds &a, const Collider::BroadPhaseBounds &b) {
        return a.min.y <= b.max.y &&
               a.max.y >= b.min.y &&
               a.min.z <= b.max.z &&
               a.max.z >= b.min.z;
    };

    std::vector<SweepEntry> entries;
    entries.reserve(bodies.size());

    for (BodyObject *body : bodies)
    {
        if (!body)
        {
            continue;
        }

        const Collider *collider = body->getCollider();
        if (!collider)
        {
            continue;
        }

        const Collider::BroadPhaseBounds bounds = collider->getBroadPhaseBounds(body->getTransform());
        if (!bounds.valid)
        {
            continue;
        }

        entries.push_back({body, collider, bounds});
    }

    for (std::size_t index = 1; index < entries.size(); ++index)
    {
        SweepEntry entry = entries[index];
        std::size_t insertIndex = index;
        while (insertIndex > 0 && entries[insertIndex - 1].bounds.min.x > entry.bounds.min.x)
        {
            entries[insertIndex] = entries[insertIndex - 1];
            --insertIndex;
        }
        entries[insertIndex] = entry;
    }

    sweepAndPruneOrder.clear();
    sweepAndPruneOrder.reserve(entries.size());
    for (const SweepEntry &entry : entries)
    {
        sweepAndPruneOrder.push_back(entry.body);
    }

    std::vector<std::size_t> activeEntries;
    activeEntries.reserve(entries.size());
    for (std::size_t entryIndex = 0; entryIndex < entries.size(); ++entryIndex)
    {
        const SweepEntry &entryA = entries[entryIndex];
        activeEntries.erase(
            std::remove_if(
                activeEntries.begin(),
                activeEntries.end(),
                [&](std::size_t activeIndex) {
                    return entries[activeIndex].bounds.max.x < entryA.bounds.min.x;
                }),
            activeEntries.end());

        for (std::size_t activeIndex : activeEntries)
        {
            const SweepEntry &entryB = entries[activeIndex];
            if (!overlapsOnYZ(entryA.bounds, entryB.bounds))
            {
                continue;
            }

            const CollisionPoints collision = entryA.collider->testCollision(
                *entryB.collider,
                entryA.body->getTransform(),
                entryB.body->getTransform());
            if (!collision.hasCollision)
            {
                continue;
            }

            const BodyPair pair = makeBodyPair(*entryA.body, *entryB.body);
            currentCollisions.insert(pair);

            if (activeCollisions.find(pair) == activeCollisions.end())
            {
                entryA.body->notifyCollisionEnter(*entryB.body, collision);
                entryB.body->notifyCollisionEnter(*entryA.body, invertCollision(collision));
            }

            entryA.body->notifyCollisionStay(*entryB.body, collision);
            entryB.body->notifyCollisionStay(*entryA.body, invertCollision(collision));
        }

        activeEntries.push_back(entryIndex);
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
