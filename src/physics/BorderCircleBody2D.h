#pragma once
#include "physics/PhysicsBody2D.h"
#include "physics/shapes/CircleShape.h"
#include <cmath>

class BorderCircleBody2D : public PhysicsBody2D
{
public:
    BorderCircleBody2D(const Vector2 &pos, float radius)
        : PhysicsBody2D(std::make_unique<CircleShape>(radius), pos), radius(radius) {}

    float getRadius() const { return radius; }
    Vector2 getCenter() const { return position; }

private:
    float radius;
};
