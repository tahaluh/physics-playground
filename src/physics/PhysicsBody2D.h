#pragma once
#include <algorithm>
#include <memory>
#include "physics/shapes/Shape.h"
#include "math/Vector2.h"

class Renderer2D;

class PhysicsBody2D
{
public:
    std::unique_ptr<Shape> shape;
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    float rotation;
    float mass;
    float linearDamping;
    float restitution;
    float surfaceFriction;
    bool useGravity;
    uint32_t color = 0xFFFFFFFF; // cor padrão: branco

    PhysicsBody2D(std::unique_ptr<Shape> shape,
                  Vector2 pos,
                  Vector2 vel = {0, 0},
                  float rot = 0.0f,
                  uint32_t color = 0xFFFFFFFF,
                  float mass = 1.0f,
                  float linearDamping = 0.0f,
                  float restitution = 1.0f,
                  float surfaceFriction = 0.0f,
                  bool useGravity = true)
        : shape(std::move(shape)),
          position(pos),
          velocity(vel),
          acceleration(Vector2::zero()),
          rotation(rot),
          mass(mass),
          linearDamping(linearDamping),
          restitution(restitution),
          surfaceFriction(surfaceFriction),
          useGravity(useGravity),
          color(color) {}

    virtual bool isStatic() const
    {
        return mass <= 0.0f;
    }

    virtual void applyForce(const Vector2 &force)
    {
        if (isStatic())
            return;
        acceleration += force / mass;
    }

    virtual void integrate(float dt)
    {
        if (isStatic())
            return;

        velocity = velocity + acceleration * dt;
        const float dampingFactor = std::max(0.0f, 1.0f - linearDamping * dt);
        velocity *= dampingFactor;
        position += velocity * dt;
        acceleration = Vector2::zero();
    }

    virtual void render(Renderer2D &renderer) const;

    virtual void onCollision(PhysicsBody2D &other)
    {
        // Padrão: não faz nada. Substitua em classes derivadas.
    }

    virtual ~PhysicsBody2D() = default;

protected:
    static bool resolveDynamicCircleCollision(PhysicsBody2D &a, PhysicsBody2D &b);
    bool resolveBorderCircleCollision(PhysicsBody2D &other, float stopThreshold = 0.0f);
    bool resolveBorderBoxCollision(PhysicsBody2D &other, float stopThreshold = 0.0f);
    bool resolveBorderCircleAxisInvertCollision(PhysicsBody2D &other);
    void applySurfaceFrictionAlongNormal(const Vector2 &normal);
};
