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

const std::vector<Manifold2D> &PhysicsWorld2D::getLastManifolds() const
{
    return lastManifolds;
}

void PhysicsWorld2D::step(Scene2D &scene, float dt) const
{
    applyGlobalForces(scene);
    integrateBodies(scene, dt);
    collisionSolver->solve(scene, lastManifolds);
}

void PhysicsWorld2D::applyGlobalForces(Scene2D &scene) const
{
    for (auto &body : scene.getBodies())
    {
        if (body->isStatic() || !body->getMaterial().useGravity)
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
