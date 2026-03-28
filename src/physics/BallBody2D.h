#include "physics/BorderCircleBody2D.h"
#pragma once
#include "physics/PhysicsBody2D.h"
#include "physics/BorderBoxBody2D.h"
#include "physics/shapes/CircleShape.h"
#include <iostream>

class BallBody2D : public PhysicsBody2D
{
public:
    using PhysicsBody2D::PhysicsBody2D;

    void onCollision(PhysicsBody2D &other) override
    {
        // Colisão com círculo oco (borda)
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
            // Reflete se a bola estiver fora ou encostando na borda (não dentro)
            if (dist >= (R - r))
            {
                Vector2 normal = toBall.normalized();
                position = center + normal * (R - r);
                velocity = velocity.reflect(normal);
                std::cout << "Bola colidiu com a borda circular e refletiu!\n";
                return;
            }
        }

        // Colisão com box (borda)
        BorderBoxBody2D *box = dynamic_cast<BorderBoxBody2D *>(&other);
        if (!box)
            return;

        CircleShape *circ = dynamic_cast<CircleShape *>(shape.get());
        if (!circ)
            return;

        float r = circ->getRadius();
        Vector2 normal(0, 0);
        bool reflected = false;

        // Checa colisão com as bordas e calcula a normal
        if (position.x - r <= box->left())
        {
            position.x = box->left() + r;
            normal = Vector2(1, 0);
            reflected = true;
        }
        else if (position.x + r >= box->right())
        {
            position.x = box->right() - r;
            normal = Vector2(-1, 0);
            reflected = true;
        }
        if (position.y - r <= box->top())
        {
            position.y = box->top() + r;
            normal = Vector2(0, 1);
            reflected = true;
        }
        else if (position.y + r >= box->bottom())
        {
            position.y = box->bottom() - r;
            normal = Vector2(0, -1);
            reflected = true;
        }
        if (reflected && (normal.x != 0 || normal.y != 0))
        {
            velocity = velocity.reflect(normal);
            std::cout << "Bola colidiu com a borda e refletiu!\n";
        }
    }
};
