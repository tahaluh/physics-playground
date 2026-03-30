#include "engine/scene/2d/Scene2D.h"

void Scene2D::addBody(std::unique_ptr<PhysicsBody2D> body)
{
    bodies.push_back(std::move(body));
}

std::vector<std::unique_ptr<PhysicsBody2D>> &Scene2D::getBodies()
{
    return bodies;
}

const std::vector<std::unique_ptr<PhysicsBody2D>> &Scene2D::getBodies() const
{
    return bodies;
}

bool Scene2D::hasAwakeDynamicBodies() const
{
    for (const auto &body : bodies)
    {
        if (body && !body->isStatic() && !body->isSleeping())
        {
            return true;
        }
    }
    return false;
}
