#pragma once

#include "engine/physics/2d/RigidBody2D.h"
#include "engine/physics/2d/shapes/RectShape.h"

class BoxBody2D : public RigidBody2D
{
public:
    BoxBody2D(
        float width,
        float height,
        Vector2 pos,
        Vector2 vel = {0, 0},
        uint32_t color = 0xFFFFFFFF)
        : RigidBody2D(
              std::make_unique<RectShape>(width, height),
              pos - Vector2(width * 0.5f, height * 0.5f),
              Vector2(1.0f, 0.0f),
              vel,
              color,
              1.0f,
              PhysicsSurfaceMaterial2D{0.82f, 0.14f, 0.08f},
              RigidBodySettings2D{0.0f, 0.6f, true}),
          width(width),
          height(height)
    {
    }

    float getWidth() const { return width; }
    float getHeight() const { return height; }

    void onCollision(const Contact2D &contact) override
    {
        if (resolveBorderCircleCollision(contact))
            return;

        resolveBorderBoxCollision(contact);
    }

private:
    float width = 0.0f;
    float height = 0.0f;
};
