#include "scene/Scene2D.h"

#include "physics/PhysicsBody2D.h"
#include "render/Renderer2D.h"

void Scene2D::addBody(std::unique_ptr<PhysicsBody2D> body)
{
    bodies.push_back(std::move(body));
}

void Scene2D::render(Renderer2D &renderer) const
{
    for (const auto &body : bodies)
    {
        body->render(renderer);
    }
}

std::vector<std::unique_ptr<PhysicsBody2D>> &Scene2D::getBodies()
{
    return bodies;
}

const std::vector<std::unique_ptr<PhysicsBody2D>> &Scene2D::getBodies() const
{
    return bodies;
}
