#pragma once
#include "engine/physics/2d/RigidBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"

class BallInvertBody2D : public RigidBody2D
{
public:
    BallInvertBody2D(float radius, Vector2 pos, Vector2 vel = {0, 0}, uint32_t color = 0xFFCC2222)
        : RigidBody2D(
              std::make_unique<CircleShape>(radius),
              pos,
              Vector2(1.0f, 0.0f),
              vel,
              color,
              1.0f,
              PhysicsSurfaceMaterial2D{1.0f, 0.05f, 0.02f},
              RigidBodySettings2D{0.0f, 0.0f, true}) {}

    void onCollision(const Contact2D &contact) override
    {
        resolveBorderCircleAxisInvertCollision(contact);
    }
};
