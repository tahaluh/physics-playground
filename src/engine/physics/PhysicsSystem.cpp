#include "engine/physics/PhysicsSystem.h"

#include <algorithm>

#include "engine/scene/objects/BodyObject.h"

void PhysicsSystem::addBody(BodyObject &body)
{
    if (body.getConfig().motionType == BodyMotionType::Static)
    {
        return;
    }

    if (std::find(bodies.begin(), bodies.end(), &body) != bodies.end())
    {
        return;
    }

    bodies.push_back(&body);
}

void PhysicsSystem::removeBody(const BodyObject &body)
{
    bodies.erase(std::remove(bodies.begin(), bodies.end(), &body), bodies.end());
}

void PhysicsSystem::clearBodies()
{
    bodies.clear();
}

bool PhysicsSystem::step(float dt) const
{
    bool anyBodyMoved = false;
    for (BodyObject *body : bodies)
    {
        if (!body)
        {
            continue;
        }

        anyBodyMoved = integrateBody(*body, dt) || anyBodyMoved;
    }

    return anyBodyMoved;
}

bool PhysicsSystem::integrateBody(BodyObject &body, float dt) const
{
    if (dt <= 0.0f)
    {
        return false;
    }

    const BodyObjectDesc &config = body.getConfig();
    if (config.motionType == BodyMotionType::Static)
    {
        return false;
    }

    const BodyPhysicsState &physicsState = body.getPhysicsState();
    if (physicsState.linearVelocity.lengthSquared() == 0.0f &&
        physicsState.angularVelocity.lengthSquared() == 0.0f)
    {
        return false;
    }

    Transform transform = body.getTransform();
    transform.position += physicsState.linearVelocity * dt;
    const Quaternion deltaRotation = Quaternion::fromEulerXYZ(physicsState.angularVelocity * dt);
    transform.rotation = (deltaRotation * transform.rotation).normalized();
    body.setTransform(transform);
    return true;
}
