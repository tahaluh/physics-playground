#pragma once
#include "physics/PhysicsBody2D.h"
#include "physics/shapes/RectShape.h"

class BorderBoxBody2D : public PhysicsBody2D
{
public:
    BorderBoxBody2D(Vector2 pos, float width, float height)
        : PhysicsBody2D(std::make_unique<RectShape>(width, height), pos, Vector2::zero(), 0.0f, 0xFFFFFFFF, 0.0f, 0.0f, 1.0f, 0.0f, false) {}

    float left() const { return position.x; }
    float right() const { return position.x + static_cast<RectShape *>(shape.get())->getWidth(); }
    float top() const { return position.y; }
    float bottom() const { return position.y + static_cast<RectShape *>(shape.get())->getHeight(); }
};
