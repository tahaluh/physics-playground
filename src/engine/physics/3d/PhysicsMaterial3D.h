#pragma once

struct PhysicsSurfaceMaterial3D
{
    float restitution = 0.85f;
    float friction = 0.0f;
};

struct RigidBodySettings3D
{
    float linearDamping = 0.02f;
    float angularDamping = 0.02f;
    bool useGravity = true;
};
