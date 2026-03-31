#pragma once

#include <memory>
#include <unordered_set>

#include "engine/physics/BroadPhase.h"

class BodyObject;

class PhysicsSystem
{
public:
    PhysicsSystem();

    void addBody(BodyObject &body);
    void removeBody(const BodyObject &body);
    void clearBodies();
    void setBroadPhaseCompute(BroadPhaseCompute *computeBackend);
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

    std::vector<BodyObject *> bodies;
    std::unordered_set<BodyPair, BodyPairHasher> activeCollisions;
    std::unique_ptr<BroadPhase> broadPhase;

    bool integrateBody(BodyObject &body, float dt);
    void resolveCollisions(float dt);
};
