#include "engine/physics/2d/PhysicsWorld2D.h"

#include "engine/physics/2d/CollisionSolver2D.h"
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/scene/2d/Scene2D.h"

PhysicsWorld2D::PhysicsWorld2D()
    : collisionSolver(std::make_unique<CollisionSolver2D>())
{
}

PhysicsWorld2D::~PhysicsWorld2D() = default;

void PhysicsWorld2D::setGravity(const Vector2 &newGravity)
{
    gravity = newGravity;
}

const Vector2 &PhysicsWorld2D::getGravity() const
{
    return gravity;
}

void PhysicsWorld2D::setSolverIterations(int newSolverIterations)
{
    solverIterations = std::max(1, newSolverIterations);
}

int PhysicsWorld2D::getSolverIterations() const
{
    return solverIterations;
}

void PhysicsWorld2D::setStopThreshold(float newStopThreshold)
{
    stopThreshold = std::max(0.0f, newStopThreshold);
}

float PhysicsWorld2D::getStopThreshold() const
{
    return stopThreshold;
}

void PhysicsWorld2D::setAngularStopThreshold(float newAngularStopThreshold)
{
    angularStopThreshold = std::max(0.0f, newAngularStopThreshold);
}

float PhysicsWorld2D::getAngularStopThreshold() const
{
    return angularStopThreshold;
}

void PhysicsWorld2D::setSleepDelay(float newSleepDelay)
{
    sleepDelay = std::max(0.0f, newSleepDelay);
}

float PhysicsWorld2D::getSleepDelay() const
{
    return sleepDelay;
}

const std::vector<Manifold2D> &PhysicsWorld2D::getLastManifolds() const
{
    return lastManifolds;
}

void PhysicsWorld2D::step(Scene2D &scene, float dt) const
{
    applyGlobalForces(scene);
    integrateBodies(scene, dt);
    for (int iteration = 0; iteration < solverIterations; ++iteration)
    {
        collisionSolver->solve(scene, lastManifolds);
    }
    updateSleeping(scene, dt);
}

void PhysicsWorld2D::applyGlobalForces(Scene2D &scene) const
{
    for (auto &body : scene.getBodies())
    {
        if (body->isStatic() || !body->getRigidBodySettings().useGravity)
            continue;

        body->applyForce(gravity * body->getMass());
    }
}

void PhysicsWorld2D::integrateBodies(Scene2D &scene, float dt) const
{
    for (auto &body : scene.getBodies())
    {
        body->integrate(dt);
    }
}

void PhysicsWorld2D::updateSleeping(Scene2D &scene, float dt) const
{
    if (sleepDelay <= 0.0f)
    {
        return;
    }

    for (auto &body : scene.getBodies())
    {
        if (!body || body->isStatic())
        {
            continue;
        }

        const bool lowLinear = stopThreshold > 0.0f && body->getVelocity().length() < stopThreshold;
        const bool lowAngular = angularStopThreshold > 0.0f && std::abs(body->getAngularVelocity()) < angularStopThreshold;
        if (lowLinear && lowAngular)
        {
            body->setSleepTime(body->getSleepTime() + dt);
            if (body->getSleepTime() >= sleepDelay)
            {
                body->setSleeping(true);
            }
        }
        else
        {
            body->setSleepTime(0.0f);
            body->setSleeping(false);
        }
    }
}
