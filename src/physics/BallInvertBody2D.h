
#pragma once
#include "physics/PhysicsBody2D.h"
#include "physics/BorderCircleBody2D.h"
#include "physics/shapes/CircleShape.h"
#include <iostream>

class BallInvertBody2D : public PhysicsBody2D
{
public:
    BallInvertBody2D(float radius, Vector2 pos, Vector2 vel = {0, 0}, uint32_t color = 0xFFCC2222)
        : PhysicsBody2D(std::make_unique<CircleShape>(radius), pos, vel, 0.0f, color, 1.0f, 0.0f) {}

    void onCollision(PhysicsBody2D &other) override
    {
        if (resolveDynamicCircleCollision(*this, other, 1.0f))
        {
            return;
        }

        BorderCircleBody2D *borderCircle = dynamic_cast<BorderCircleBody2D *>(&other);
        if (borderCircle)
        {
            CircleShape *circ = dynamic_cast<CircleShape *>(shape.get());
            if (!circ)
                return;
            float r = circ->getRadius();
            float R = borderCircle->getRadius();
            Vector2 center = borderCircle->getCenter();
            Vector2 toBall = position - center;
            float dist = toBall.length();
            if (dist >= (R - r))
            {
                // Inverte apenas o eixo dominante
                Vector2 normal = toBall.normalized();
                if (std::abs(normal.x) > std::abs(normal.y))
                {
                    velocity.x = -velocity.x;
                }
                else
                {
                    velocity.y = -velocity.y;
                }
                // Corrige posição
                position = center + normal * (R - r);
                std::cout << "Bola (invert) colidiu com a borda circular!\n";
                return;
            }
        }
    }
};
