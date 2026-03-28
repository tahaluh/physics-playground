#pragma once
#include "physics/PhysicsBody2D.h"
#include "physics/shapes/CircleShape.h"

class BallBody2D : public PhysicsBody2D
{
public:
    BallBody2D(float radius, Vector2 pos, Vector2 vel = {0, 0}, uint32_t color = 0xFFFFFFFF)
        : PhysicsBody2D(std::make_unique<CircleShape>(radius), pos, vel, 0.0f, color, 1.0f, 0.0f, 0.9f, true) {}

    void onCollision(PhysicsBody2D &other) override
    {
        constexpr float stopThreshold = 25.0f;

        if (resolveDynamicCircleCollision(*this, other))
        {
            return;
        }

        if (resolveBorderCircleCollision(other, stopThreshold))
            return;

        resolveBorderBoxCollision(other, stopThreshold);
    }
};
