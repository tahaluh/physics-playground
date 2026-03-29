#pragma once

struct PhysicsSurfaceMaterial2D
{
    float restitution = 1.0f;
    float staticFriction = 0.0f;
    float dynamicFriction = 0.0f;
};

struct RigidBodySettings2D
{
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    bool useGravity = true;
};
