#pragma once
#include <algorithm>
#include <memory>
#include "engine/physics/2d/Contact2D.h"
#include "engine/physics/2d/Material2D.h"
#include "engine/physics/2d/shapes/Shape.h"
#include "engine/math/Vector2.h"

class PhysicsBody2D
{
public:
    PhysicsBody2D(std::unique_ptr<Shape> shape,
                  Vector2 pos,
                  Vector2 rot = Vector2(1.0f, 0.0f),
                  Vector2 vel = {0, 0},
                  uint32_t color = 0xFFFFFFFF,
                  float mass = 1.0f,
                  PhysicsSurfaceMaterial2D surfaceMaterial = {},
                  RigidBodySettings2D rigidBodySettings = {})
        : shape(std::move(shape)),
          position(pos),
          rotation(rot.lengthSquared() > 0.0f ? rot.normalized() : Vector2(1.0f, 0.0f)),
          velocity(vel),
          acceleration(Vector2::zero()),
          mass(mass),
          inverseMass(mass > 0.0f ? 1.0f / mass : 0.0f),
          surfaceMaterial(surfaceMaterial),
          rigidBodySettings(rigidBodySettings),
          momentOfInertia(computeMomentOfInertia()),
          inverseMomentOfInertia(momentOfInertia > 0.0f ? 1.0f / momentOfInertia : 0.0f),
          color(color) {}

    Shape *getShape() { return shape.get(); }
    const Shape *getShape() const { return shape.get(); }

    const Vector2 &getPosition() const { return position; }
    void setPosition(const Vector2 &newPosition) { position = newPosition; }

    const Vector2 &getVelocity() const { return velocity; }
    void setVelocity(const Vector2 &newVelocity) { velocity = newVelocity; }

    const Vector2 &getAcceleration() const { return acceleration; }
    void setAcceleration(const Vector2 &newAcceleration) { acceleration = newAcceleration; }

    const Vector2 &getRotation() const { return rotation; }
    void setRotation(const Vector2 &newRotation)
    {
        rotation = newRotation.lengthSquared() > 0.0f ? newRotation.normalized() : Vector2(1.0f, 0.0f);
    }
    float getRotationAngle() const { return std::atan2(rotation.y, rotation.x); }

    float getMass() const { return mass; }
    float getInverseMass() const { return inverseMass; }
    float getMomentOfInertia() const { return momentOfInertia; }
    float getInverseMomentOfInertia() const { return inverseMomentOfInertia; }
    const PhysicsSurfaceMaterial2D &getSurfaceMaterial() const { return surfaceMaterial; }
    PhysicsSurfaceMaterial2D &getSurfaceMaterial() { return surfaceMaterial; }
    const RigidBodySettings2D &getRigidBodySettings() const { return rigidBodySettings; }
    RigidBodySettings2D &getRigidBodySettings() { return rigidBodySettings; }

    float getAngularVelocity() const { return angularVelocity; }
    void setAngularVelocity(float newAngularVelocity) { angularVelocity = newAngularVelocity; }
    float getTorque() const { return torque; }
    void setTorque(float newTorque) { torque = newTorque; }

    uint32_t getColor() const { return color; }
    void setColor(uint32_t newColor) { color = newColor; }
    bool isCollisionEnabled() const { return collisionEnabled; }
    void setCollisionEnabled(bool enabled) { collisionEnabled = enabled; }

    virtual bool isStatic() const
    {
        return mass <= 0.0f;
    }

    virtual void applyForce(const Vector2 &force)
    {
        if (isStatic())
            return;
        acceleration += force * inverseMass;
    }

    virtual void applyForceAtPoint(const Vector2 &force, const Vector2 &point)
    {
        if (isStatic())
            return;

        applyForce(force);
        const Vector2 leverArm = point - position;
        torque += Vector2::cross(leverArm, force);
    }

    virtual void integrate(float dt)
    {
        if (isStatic())
            return;

        velocity = velocity + acceleration * dt;
        const float linearDampingFactor = std::max(0.0f, 1.0f - rigidBodySettings.linearDamping * dt);
        velocity *= linearDampingFactor;
        position += velocity * dt;
        angularVelocity += torque * inverseMomentOfInertia * dt;
        const float angularDampingFactor = std::max(0.0f, 1.0f - rigidBodySettings.angularDamping * dt);
        angularVelocity *= angularDampingFactor;
        const float angle = getRotationAngle() + angularVelocity * dt;
        rotation = Vector2(std::cos(angle), std::sin(angle));
        acceleration = Vector2::zero();
        torque = 0.0f;
    }

    virtual void onCollision(const Contact2D &contact)
    {
        // Padrão: não faz nada. Substitua em classes derivadas.
    }

    virtual ~PhysicsBody2D() = default;

protected:
    bool resolveBorderCircleCollision(const Contact2D &contact, float stopThreshold = 0.0f);
    bool resolveBorderBoxCollision(const Contact2D &contact, float stopThreshold = 0.0f);
    bool resolveBorderCircleAxisInvertCollision(const Contact2D &contact);
    void applySurfaceFrictionAlongNormal(const Vector2 &normal, const Vector2 &contactOffset);
    void applySurfaceFrictionAlongNormal(const Vector2 &normal, const Vector2 &contactOffset, float staticFriction, float dynamicFriction);
    float computeMomentOfInertia() const;
    Vector2 getCenterOfMassPosition() const;

private:
    std::unique_ptr<Shape> shape;
    Vector2 position;
    Vector2 rotation;
    Vector2 velocity;
    Vector2 acceleration;
    float mass;
    float inverseMass;
    PhysicsSurfaceMaterial2D surfaceMaterial;
    RigidBodySettings2D rigidBodySettings;
    float angularVelocity = 0.0f;
    float torque = 0.0f;
    float momentOfInertia = 0.0f;
    float inverseMomentOfInertia = 0.0f;
    uint32_t color = 0xFFFFFFFF; // cor padrão: branco
    bool collisionEnabled = true;
};
