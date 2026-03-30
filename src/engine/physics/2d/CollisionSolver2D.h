#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

class PhysicsBody2D;
struct Contact2D;
struct Manifold2D;
class Scene2D;

class CollisionSolver2D
{
public:
    void solve(Scene2D &scene, std::vector<Manifold2D> &manifolds) const;

private:
    struct CachedContactPair2D
    {
        float normalImpulseMagnitude = 0.0f;
        float tangentImpulseMagnitude = 0.0f;
        uint64_t generation = 0;
    };

    bool buildManifold(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, Manifold2D &manifold) const;
    void buildContactsFromManifold(const Manifold2D &manifold, Contact2D &contactA, Contact2D &contactB) const;
    bool resolvePair(
        PhysicsBody2D &bodyA,
        PhysicsBody2D &bodyB,
        const Contact2D &contactA,
        const Contact2D &contactB,
        float *outNormalImpulseMagnitude) const;
    bool resolveDynamicCircleCollision(const Contact2D &contactA, const Contact2D &contactB, float *outNormalImpulseMagnitude) const;
    mutable std::unordered_map<uint64_t, CachedContactPair2D> persistentContactPairs;
    mutable uint64_t contactGeneration = 0;
};
