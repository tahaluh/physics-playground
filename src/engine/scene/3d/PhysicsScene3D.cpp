#include "engine/scene/3d/PhysicsScene3D.h"

void PhysicsScene3D::addBody(std::unique_ptr<PhysicsBody3D> body)
{
    bodies.push_back(std::move(body));
}

std::vector<std::unique_ptr<PhysicsBody3D>> &PhysicsScene3D::getBodies()
{
    return bodies;
}

const std::vector<std::unique_ptr<PhysicsBody3D>> &PhysicsScene3D::getBodies() const
{
    return bodies;
}
