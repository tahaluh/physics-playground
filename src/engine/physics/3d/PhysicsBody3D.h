#pragma once

#include <algorithm>
#include <memory>

#include "engine/math/Vector3.h"
#include "engine/physics/3d/PhysicsMaterial3D.h"
#include "engine/physics/3d/shapes/Shape3D.h"

class PhysicsBody3D
{
public:
    PhysicsBody3D(
        std::unique_ptr<Shape3D> shape,
        Vector3 position,
        Vector3 velocity = Vector3::zero(),
        float mass = 1.0f,
        PhysicsMaterial3D material = {})
        : shape(std::move(shape)),
          position(position),
          velocity(velocity),
          acceleration(Vector3::zero()),
          mass(mass),
          material(material)
    {
    }

    Shape3D *getShape() { return shape.get(); }
    const Shape3D *getShape() const { return shape.get(); }

    const Vector3 &getPosition() const { return position; }
    void setPosition(const Vector3 &newPosition) { position = newPosition; }

    const Vector3 &getVelocity() const { return velocity; }
    void setVelocity(const Vector3 &newVelocity) { velocity = newVelocity; }

    const Vector3 &getAcceleration() const { return acceleration; }
    void setAcceleration(const Vector3 &newAcceleration) { acceleration = newAcceleration; }

    float getMass() const { return mass; }
    const PhysicsMaterial3D &getMaterial() const { return material; }
    PhysicsMaterial3D &getMaterial() { return material; }

    virtual bool isStatic() const
    {
        return mass <= 0.0f;
    }

    virtual void applyForce(const Vector3 &force)
    {
        if (isStatic())
            return;

        acceleration += force / mass;
    }

    virtual void integrate(float dt)
    {
        if (isStatic())
            return;

        velocity += acceleration * dt;
        const float dampingFactor = std::max(0.0f, 1.0f - material.linearDamping * dt);
        velocity *= dampingFactor;
        position += velocity * dt;
        acceleration = Vector3::zero();
    }

    virtual ~PhysicsBody3D() = default;

private:
    std::unique_ptr<Shape3D> shape;
    Vector3 position;
    Vector3 velocity;
    Vector3 acceleration;
    float mass = 1.0f;
    PhysicsMaterial3D material;
};
