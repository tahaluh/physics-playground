#pragma once
#include "physics/PhysicsBody2D.h"
#include "physics/shapes/CircleShape.h"

class BallInvertBody2D : public PhysicsBody2D
{
public:
    BallInvertBody2D(float radius, Vector2 pos, Vector2 vel = {0, 0}, uint32_t color = 0xFFCC2222)
        : PhysicsBody2D(std::make_unique<CircleShape>(radius), pos, vel, 0.0f, color, 1.0f, 0.0f, 1.0f, 0.02f, true) {}

    void onCollision(PhysicsBody2D &other) override
    {
        if (resolveDynamicCircleCollision(*this, other))
        {
            return;
        }

        resolveBorderCircleAxisInvertCollision(other);
    }
};
