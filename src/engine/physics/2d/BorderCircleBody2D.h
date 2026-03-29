#pragma once
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include <cmath>

class BorderCircleBody2D : public PhysicsBody2D
{
public:
    BorderCircleBody2D(const Vector2 &pos, float radius)
        : PhysicsBody2D(
              std::make_unique<CircleShape>(radius),
              pos,
              Vector2(1.0f, 0.0f),
              Vector2::zero(),
              0xFFFFFFFF,
              0.0f,
              PhysicsSurfaceMaterial2D{1.0f, 0.05f, 0.0f},
              RigidBodySettings2D{0.0f, 0.0f, false}),
          radius(radius) {}

    float getRadius() const { return radius; }
    Vector2 getCenter() const { return getPosition(); }

private:
    float radius;
};
