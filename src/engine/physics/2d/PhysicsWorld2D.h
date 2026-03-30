#pragma once

#include <memory>
#include <vector>

#include "engine/math/Vector2.h"
#include "engine/physics/2d/Manifold2D.h"

class CollisionSolver2D;
class Scene2D;

class PhysicsWorld2D
{
public:
    PhysicsWorld2D();
    ~PhysicsWorld2D();

    void setGravity(const Vector2 &newGravity);
    const Vector2 &getGravity() const;
    void setSolverIterations(int newSolverIterations);
    int getSolverIterations() const;
    void setStopThreshold(float newStopThreshold);
    float getStopThreshold() const;
    void setAngularStopThreshold(float newAngularStopThreshold);
    float getAngularStopThreshold() const;
    void setSleepDelay(float newSleepDelay);
    float getSleepDelay() const;
    const std::vector<Manifold2D> &getLastManifolds() const;

    void step(Scene2D &scene, float dt) const;

private:
    void applyGlobalForces(Scene2D &scene) const;
    void integrateBodies(Scene2D &scene, float dt) const;
    void updateSleeping(Scene2D &scene, float dt) const;

    Vector2 gravity = Vector2::zero();
    int solverIterations = 6;
    float stopThreshold = 0.0f;
    float angularStopThreshold = 0.0f;
    float sleepDelay = 0.0f;
    std::unique_ptr<CollisionSolver2D> collisionSolver;
    mutable std::vector<Manifold2D> lastManifolds;
};
