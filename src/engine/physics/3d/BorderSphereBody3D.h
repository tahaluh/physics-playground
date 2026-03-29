#pragma once

#include "engine/physics/3d/PhysicsBody3D.h"
#include "engine/physics/3d/shapes/SphereShape3D.h"

class BorderSphereBody3D : public PhysicsBody3D
{
public:
    BorderSphereBody3D(const Vector3 &position, float radius)
        : PhysicsBody3D(
              std::make_unique<SphereShape3D>(radius),
              position,
              Vector3::zero(),
              Vector3::zero(),
              0.0f,
              PhysicsSurfaceMaterial3D{0.55f, 0.6f},
              RigidBodySettings3D{0.0f, 0.0f, false}),
          radius(radius)
    {
    }

    float getRadius() const { return radius; }

private:
    float radius = 0.0f;
};
