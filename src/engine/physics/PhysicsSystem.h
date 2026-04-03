#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "engine/physics/BroadPhase.h"
#include "engine/physics/CollisionPoints.h"

class BodyObject;

class PhysicsSystem
{
public:
    PhysicsSystem();

    void addBody(BodyObject &body);
    void removeBody(const BodyObject &body);
    void clearBodies();
    bool step(float dt);

private:
    struct BodyPair
    {
        BodyObject *a = nullptr;
        BodyObject *b = nullptr;

        bool operator==(const BodyPair &other) const
        {
            return a == other.a && b == other.b;
        }
    };

    struct BodyPairHasher
    {
        std::size_t operator()(const BodyPair &pair) const
        {
            const std::size_t hashA = std::hash<BodyObject *>{}(pair.a);
            const std::size_t hashB = std::hash<BodyObject *>{}(pair.b);
            return hashA ^ (hashB << 1);
        }
    };

    struct Contact
    {
        BodyObject *a = nullptr;
        BodyObject *b = nullptr;
        CollisionPoints collision;
    };

    std::vector<BodyObject *> bodies;
    std::vector<BroadPhasePair> broadPhasePairs;
    std::vector<BodyObject *> broadPhaseBodies;
    std::vector<Contact> detectedContacts;
    std::unordered_set<BodyPair, BodyPairHasher> activeCollisions;
    std::unique_ptr<BroadPhase> broadPhase;
    bool bodiesDirty = true;

    void broadPhasePass();
    bool integrateBodies(float dt);
    void narrowPhase();
    void updateCollisionCallbacks();
    const std::vector<BodyObject *> &buildBroadPhaseBodies();
};
