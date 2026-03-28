#pragma once
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include <cmath>

class BorderCircleBody2D : public PhysicsBody2D
{
public:
    BorderCircleBody2D(const Vector2 &pos, float radius)
        : PhysicsBody2D(std::make_unique<CircleShape>(radius), pos, Vector2::zero(), 0.0f, 0xFFFFFFFF, 0.0f, Material2D{0.0f, 1.0f, 0.0f, false}), radius(radius) {}

    float getRadius() const { return radius; }
    Vector2 getCenter() const { return getPosition(); }

    void render(Renderer2D &renderer) const override;

private:
    float radius;
};
