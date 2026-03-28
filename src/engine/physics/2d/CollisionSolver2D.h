#pragma once

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
    bool buildManifold(PhysicsBody2D &bodyA, PhysicsBody2D &bodyB, Manifold2D &manifold) const;
    void buildContactsFromManifold(const Manifold2D &manifold, Contact2D &contactA, Contact2D &contactB) const;
    bool resolveDynamicCircleCollision(const Contact2D &contactA, const Contact2D &contactB) const;
};
