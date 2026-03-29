#pragma once

#include "engine/math/Vector3.h"

class PhysicsScene3D;

class PhysicsWorld3D
{
public:
    void setGravity(const Vector3 &newGravity);
    const Vector3 &getGravity() const;
    void setStopThreshold(float newStopThreshold);
    float getStopThreshold() const;
    void setAngularStopThreshold(float newAngularStopThreshold);
    float getAngularStopThreshold() const;

    void step(PhysicsScene3D &scene, float dt) const;

private:
    void applyGlobalForces(PhysicsScene3D &scene) const;
    void integrateBodies(PhysicsScene3D &scene, float dt) const;
    void solveBoundaryCollisions(PhysicsScene3D &scene) const;

    Vector3 gravity = Vector3::zero();
    float stopThreshold = 0.0f;
    float angularStopThreshold = 0.0f;
};
