#pragma once
#include "engine/physics/2d/RigidBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"

class BallBody2D : public RigidBody2D
{
public:
    BallBody2D(float radius, Vector2 pos, Vector2 vel = {0, 0}, uint32_t color = 0xFFFFFFFF)
        : RigidBody2D(
              std::make_unique<CircleShape>(radius),
              pos,
              Vector2(1.0f, 0.0f),
              vel,
              color,
              1.0f,
              PhysicsSurfaceMaterial2D{0.9f, 0.14f, 0.08f},
              RigidBodySettings2D{0.0f, 0.0f, true}) {}

    void onCollision(const Contact2D &contact) override
    {
        if (resolveBorderCircleCollision(contact))
            return;

        resolveBorderBoxCollision(contact);
    }
};
