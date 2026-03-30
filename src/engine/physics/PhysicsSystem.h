#pragma once

#include <vector>

class BodyObject;

class PhysicsSystem
{
public:
    void addBody(BodyObject &body);
    void removeBody(const BodyObject &body);
    void clearBodies();
    bool step(float dt) const;

private:
    std::vector<BodyObject *> bodies;

    bool integrateBody(BodyObject &body, float dt) const;
};
