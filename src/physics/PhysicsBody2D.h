#pragma once
#include <memory>
#include "physics/shapes/Shape.h"
#include "math/Vector2.h"

class PhysicsBody2D
{
public:
    std::unique_ptr<Shape> shape;
    Vector2 position;
    Vector2 velocity;
    float rotation;
    uint32_t color = 0xFFFFFFFF; // cor padrão: branco

    PhysicsBody2D(std::unique_ptr<Shape> shape, Vector2 pos, Vector2 vel = {0, 0}, float rot = 0.0f, uint32_t color = 0xFFFFFFFF)
        : shape(std::move(shape)), position(pos), velocity(vel), rotation(rot), color(color) {}

    virtual void onCollision(PhysicsBody2D &other)
    {
        // Padrão: não faz nada. Substitua em classes derivadas.
    }

    virtual ~PhysicsBody2D() = default;
};
