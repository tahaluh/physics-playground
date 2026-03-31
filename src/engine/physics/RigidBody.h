#pragma once

#include "engine/math/Vector3.h"

struct RigidBody
{
    float mass = 1.0f;
    float restitution = 0.0f;
    float gravityScale = 1.0f;
    Vector3 linearVelocity = Vector3::zero();
    Vector3 angularVelocity = Vector3::zero();
    float sleepTime = 0.0f;
    bool sleeping = false;
    bool canSleep = true;
    bool useGravity = true;

    float getInverseMass() const
    {
        return mass > 0.0f ? 1.0f / mass : 0.0f;
    }
};
