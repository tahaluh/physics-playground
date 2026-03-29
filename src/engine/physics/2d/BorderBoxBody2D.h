#pragma once
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/shapes/RectShape.h"

class BorderBoxBody2D : public PhysicsBody2D
{
public:
    BorderBoxBody2D(Vector2 pos, float width, float height)
        : PhysicsBody2D(
              std::make_unique<RectShape>(width, height),
              pos,
              Vector2(1.0f, 0.0f),
              Vector2::zero(),
              0xFFFFFFFF,
              0.0f,
              PhysicsSurfaceMaterial2D{1.0f, 0.05f, 0.0f},
              RigidBodySettings2D{0.0f, 0.0f, false}) {}

    float left() const { return getPosition().x; }
    float right() const { return getPosition().x + static_cast<const RectShape *>(getShape())->getWidth(); }
    float top() const { return getPosition().y; }
    float bottom() const { return getPosition().y + static_cast<const RectShape *>(getShape())->getHeight(); }
};
