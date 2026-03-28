#pragma once
#include "physics/PhysicsBody2D.h"
#include "physics/shapes/CircleShape.h"
#include <cmath>

class BorderCircleBody2D : public PhysicsBody2D
{
public:
    BorderCircleBody2D(const Vector2 &pos, float radius)
        : PhysicsBody2D(std::make_unique<CircleShape>(radius), pos, Vector2::zero(), 0.0f, 0xFFFFFFFF, 0.0f, 0.0f, 1.0f, false), radius(radius) {}

    float getRadius() const { return radius; }
    Vector2 getCenter() const { return position; }

    void render(Renderer2D &renderer) const override;

private:
    float radius;
};
