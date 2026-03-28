#pragma once

#include <memory>
#include <vector>

#include "engine/physics/2d/PhysicsBody2D.h"

class Renderer2D;

class Scene2D
{
public:
    void addBody(std::unique_ptr<PhysicsBody2D> body);
    void render(Renderer2D &renderer) const;

    std::vector<std::unique_ptr<PhysicsBody2D>> &getBodies();
    const std::vector<std::unique_ptr<PhysicsBody2D>> &getBodies() const;

private:
    std::vector<std::unique_ptr<PhysicsBody2D>> bodies;
};
