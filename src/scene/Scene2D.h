

#pragma once
#include <vector>
#include <memory>
#include "physics/PhysicsBody2D.h"
#include "physics/shapes/Shape.h"
#include "physics/shapes/CircleShape.h"
#include "physics/shapes/RectShape.h"
#include "physics/BorderCircleBody2D.h"

class Scene2D
{
public:
    void addBody(std::unique_ptr<PhysicsBody2D> body)
    {
        bodies.push_back(std::move(body));
    }

    void update(float dt)
    {
        for (auto &body : bodies)
        {
            body->position = body->position + body->velocity * dt;
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
                auto &pa = bodies[i]->position;
                auto &pb = bodies[j]->position;
                if (sa && sb && sa->collidesWith(*sb, pa, pb))
                {
                    // Colisão detectada entre bodies[i] e bodies[j]
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
            Shape *shape = body->shape.get();
            // Detecta se é BorderCircleBody2D para desenhar só a borda
            if (dynamic_cast<const BorderCircleBody2D *>(body.get()))
            {
                auto *circ = static_cast<CircleShape *>(shape);
                renderer.drawCircle(
                    body->position.x,
                    body->position.y,
                    circ->getRadius(),
                    0xFFFFFFFF); // cor branca para borda
            }
            else if (shape->getType() == ShapeType::Rect)
            {
                auto *rect = static_cast<RectShape *>(shape);
                renderer.drawRect(
                    body->position.x,
                    body->position.y,
                    rect->getWidth(),
                    rect->getHeight(),
                    0xFFFFFFFF);
            }
            else if (shape->getType() == ShapeType::Circle)
            {
                auto *circ = static_cast<CircleShape *>(shape);
                renderer.drawCircle(
                    body->position.x,
                    body->position.y,
                    circ->getRadius(),
                    0xFFFFFFFF);
            }
        }
    }

    const std::vector<std::unique_ptr<PhysicsBody2D>> &getBodies() const
    {
        return bodies;
    }

private:
    std::vector<std::unique_ptr<PhysicsBody2D>> bodies;
};
