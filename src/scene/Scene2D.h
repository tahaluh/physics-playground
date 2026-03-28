#pragma once
#include <memory>
#include <vector>
#include "physics/PhysicsBody2D.h"
#include "render/Renderer2D.h"

class Scene2D
{
public:
    void setGravity(const Vector2 &newGravity)
    {
        gravity = newGravity;
    }

    void addBody(std::unique_ptr<PhysicsBody2D> body)
    {
        bodies.push_back(std::move(body));
    }

    void update(float dt)
    {
        for (auto &body : bodies)
        {
            if (!body->isStatic())
            {
                if (body->useGravity)
                {
                    body->applyForce(gravity * body->mass);
                }
            }
            body->integrate(dt);
        }
    }

    void checkCollisions()
    {
        for (size_t i = 0; i < bodies.size(); ++i)
        {
            for (size_t j = i + 1; j < bodies.size(); ++j)
            {
                Shape *sa = bodies[i]->shape.get();
                Shape *sb = bodies[j]->shape.get();
                if (sa && sb && sa->collidesWith(*sb, bodies[i]->position, bodies[j]->position))
                {
                    bodies[i]->onCollision(*bodies[j]);
                    bodies[j]->onCollision(*bodies[i]);
                }
            }
        }
    }

    void render(Renderer2D &renderer) const
    {
        for (const auto &body : bodies)
        {
            body->render(renderer);
        }
    }

    const std::vector<std::unique_ptr<PhysicsBody2D>> &getBodies() const
    {
        return bodies;
    }

private:
    std::vector<std::unique_ptr<PhysicsBody2D>> bodies;
    Vector2 gravity = Vector2(0.0f, 0.0f);
};
