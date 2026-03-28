#pragma once
#include <algorithm>
#include <memory>
#include "engine/physics/2d/Contact2D.h"
#include "engine/physics/2d/Material2D.h"
#include "engine/physics/2d/shapes/Shape.h"
#include "engine/math/Vector2.h"

class Renderer2D;

class PhysicsBody2D
{
public:
    PhysicsBody2D(std::unique_ptr<Shape> shape,
                  Vector2 pos,
                  Vector2 vel = {0, 0},
                  float rot = 0.0f,
                  uint32_t color = 0xFFFFFFFF,
                  float mass = 1.0f,
                  Material2D material = {})
        : shape(std::move(shape)),
          position(pos),
          velocity(vel),
          acceleration(Vector2::zero()),
          rotation(rot),
          mass(mass),
          material(material),
          color(color) {}

    Shape *getShape() { return shape.get(); }
    const Shape *getShape() const { return shape.get(); }

    const Vector2 &getPosition() const { return position; }
    void setPosition(const Vector2 &newPosition) { position = newPosition; }

    const Vector2 &getVelocity() const { return velocity; }
    void setVelocity(const Vector2 &newVelocity) { velocity = newVelocity; }

    const Vector2 &getAcceleration() const { return acceleration; }
    void setAcceleration(const Vector2 &newAcceleration) { acceleration = newAcceleration; }

    float getRotation() const { return rotation; }
    void setRotation(float newRotation) { rotation = newRotation; }

    float getMass() const { return mass; }
    const Material2D &getMaterial() const { return material; }
    Material2D &getMaterial() { return material; }

    uint32_t getColor() const { return color; }
    void setColor(uint32_t newColor) { color = newColor; }

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
        const float dampingFactor = std::max(0.0f, 1.0f - material.linearDamping * dt);
        velocity *= dampingFactor;
        position += velocity * dt;
        acceleration = Vector2::zero();
    }

    virtual void render(Renderer2D &renderer) const;

    virtual void onCollision(const Contact2D &contact)
    {
        // Padrão: não faz nada. Substitua em classes derivadas.
    }

    virtual ~PhysicsBody2D() = default;

protected:
    bool resolveBorderCircleCollision(const Contact2D &contact, float stopThreshold = 0.0f);
    bool resolveBorderBoxCollision(const Contact2D &contact, float stopThreshold = 0.0f);
    bool resolveBorderCircleAxisInvertCollision(const Contact2D &contact);
    void applySurfaceFrictionAlongNormal(const Vector2 &normal);

private:
    std::unique_ptr<Shape> shape;
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    float rotation;
    float mass;
    Material2D material;
    uint32_t color = 0xFFFFFFFF; // cor padrão: branco
};
