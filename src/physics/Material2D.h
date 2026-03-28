#pragma once

struct Material2D
{
    float linearDamping = 0.0f;
    float restitution = 1.0f;
    float surfaceFriction = 0.0f;
    bool useGravity = true;
};
