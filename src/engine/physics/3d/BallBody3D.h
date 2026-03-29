#pragma once

#include "engine/physics/3d/PhysicsBody3D.h"
#include "engine/physics/3d/shapes/SphereShape3D.h"

class BallBody3D : public PhysicsBody3D
{
public:
    BallBody3D(float radius, const Vector3 &position, const Vector3 &velocity = Vector3::zero())
        : PhysicsBody3D(
              std::make_unique<SphereShape3D>(radius),
              position,
              velocity,
              1.0f,
              PhysicsMaterial3D{0.02f, 0.88f, true})
    {
    }
};
