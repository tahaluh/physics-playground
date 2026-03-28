#pragma once
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"

class BallBody2D : public PhysicsBody2D
{
public:
    BallBody2D(float radius, Vector2 pos, Vector2 vel = {0, 0}, uint32_t color = 0xFFFFFFFF)
        : PhysicsBody2D(std::make_unique<CircleShape>(radius), pos, vel, 0.0f, color, 1.0f, Material2D{0.0f, 0.9f, 0.08f, true}) {}

    void onCollision(const Contact2D &contact) override
    {
        constexpr float stopThreshold = 25.0f;

        if (resolveBorderCircleCollision(contact, stopThreshold))
            return;

        resolveBorderBoxCollision(contact, stopThreshold);
    }
};
