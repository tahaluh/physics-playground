#pragma once

struct PhysicsMaterial3D
{
    float linearDamping = 0.02f;
    float restitution = 0.85f;
    bool useGravity = true;
};
