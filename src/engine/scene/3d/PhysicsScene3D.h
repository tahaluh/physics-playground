#pragma once

#include <memory>
#include <vector>

#include "engine/physics/3d/PhysicsBody3D.h"

class PhysicsScene3D
{
public:
    void addBody(std::unique_ptr<PhysicsBody3D> body);

    std::vector<std::unique_ptr<PhysicsBody3D>> &getBodies();
    const std::vector<std::unique_ptr<PhysicsBody3D>> &getBodies() const;
    bool hasAwakeDynamicBodies() const;

private:
    std::vector<std::unique_ptr<PhysicsBody3D>> bodies;
};
