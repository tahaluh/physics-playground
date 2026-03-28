#pragma once
#include "physics/PhysicsBody2D.h"
#include "physics/shapes/CircleShape.h"

class BallInvertBody2D : public PhysicsBody2D
{
public:
    BallInvertBody2D(float radius, Vector2 pos, Vector2 vel = {0, 0}, uint32_t color = 0xFFCC2222)
        : PhysicsBody2D(std::make_unique<CircleShape>(radius), pos, vel, 0.0f, color, 1.0f, Material2D{0.0f, 1.0f, 0.02f, true}) {}

    void onCollision(const Contact2D &contact) override
    {
        resolveBorderCircleAxisInvertCollision(contact);
    }
};
