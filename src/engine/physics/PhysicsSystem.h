#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

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

    struct Contact
    {
        BodyObject *a = nullptr;
        BodyObject *b = nullptr;
        CollisionPoints collision;
    };

    struct ContactManifold
    {
        BodyObject *a = nullptr;
        BodyObject *b = nullptr;
        std::vector<CollisionPoints> points;
        CollisionPoints representative;
        int persistence = 0;
    };

    std::vector<BodyObject *> bodies;
    std::vector<BodyObject *> activeBodies;
    std::vector<BroadPhasePair> broadPhasePairs;
    std::vector<Contact> detectedContacts;
    std::vector<Contact> contacts;
    std::unordered_map<BodyPair, ContactManifold, BodyPairHasher> manifolds;
    std::unordered_set<BodyPair, BodyPairHasher> activeCollisions;
    std::unique_ptr<BroadPhase> broadPhase;

    void applyGlobalForces();
    void integrateVelocities(float dt);
    void broadPhasePass();
    void narrowPhase();
    void buildContacts();
    void solveVelocityConstraints(int iterations, float dt);
    bool integratePositions(float dt);
    bool solvePositionConstraints(int iterations);
    void updateCollisionCallbacks();
    void updateSleep(float dt);
    void clearAccumulators();
    void refreshActiveBodies();
    std::vector<BodyObject *> buildBroadPhaseBodies() const;
    bool hasAwakeDynamicBodies() const;
    bool isBodyPassiveForCurrentStep(const BodyObject &body) const;
    bool canBodySleepOnCurrentContacts(const BodyObject &body) const;
    bool getStableSupportNormal(const BodyObject &body, Vector3 &supportNormal) const;
    void solveVelocityConstraint(BodyObject &bodyA, BodyObject &bodyB, const CollisionPoints &collision, float dt);
    bool solvePositionConstraint(BodyObject &bodyA, BodyObject &bodyB, const CollisionPoints &collision);
};
