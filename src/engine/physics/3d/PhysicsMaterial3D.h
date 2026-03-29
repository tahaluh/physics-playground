#pragma once

#include "engine/math/Vector3.h"

struct PhysicsSurfaceMaterial3D
{
    float restitution = 0.85f;
    float staticFriction = 0.0f;
    float dynamicFriction = 0.0f;
};

struct RigidBodySettings3D
{
    float linearDamping = 0.02f;
    float angularDamping = 0.02f;
    bool useGravity = true;
    Vector3 centerOfMassOffset = Vector3::zero();
};
