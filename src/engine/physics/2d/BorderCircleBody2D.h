#pragma once
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include <cmath>

class BorderCircleBody2D : public PhysicsBody2D
{
public:
    BorderCircleBody2D(const Vector2 &pos, float innerRadius, float outerRadius)
        : PhysicsBody2D(
              std::make_unique<CircleShape>(outerRadius),
              pos,
              Vector2(1.0f, 0.0f),
              Vector2::zero(),
              0xFFFFFFFF,
              0.0f,
              PhysicsSurfaceMaterial2D{0.2f, 0.8f, 0.55f},
              RigidBodySettings2D{0.0f, 0.0f, false}),
          innerRadius(std::max(0.0f, std::min(innerRadius, outerRadius))),
          outerRadius(std::max(0.0f, outerRadius)) {}

    float getInnerRadius() const { return innerRadius; }
    float getOuterRadius() const { return outerRadius; }
    Vector2 getCenter() const { return getPosition(); }

private:
    float innerRadius;
    float outerRadius;
};
