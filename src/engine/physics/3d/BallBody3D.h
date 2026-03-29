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
              Vector3::zero(),
              velocity,
              1.0f,
              PhysicsSurfaceMaterial3D{0.82f, 0.62f, 0.55f},
              RigidBodySettings3D{0.02f, 0.02f, true})
    {
    }
};
