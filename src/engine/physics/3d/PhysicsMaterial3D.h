#pragma once

struct PhysicsMaterial3D
{
    float linearDamping = 0.02f;
    float restitution = 0.85f;
    float surfaceFriction = 0.0f;
    bool useGravity = true;
};
