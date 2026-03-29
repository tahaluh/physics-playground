#pragma once

#include <algorithm>
#include <memory>

#include "engine/math/Matrix4.h"
#include "engine/math/Quaternion.h"
#include "engine/math/Vector3.h"
#include "engine/physics/3d/PhysicsMaterial3D.h"
#include "engine/physics/3d/shapes/BoxCollider3D.h"
#include "engine/physics/3d/shapes/SphereShape3D.h"
#include "engine/physics/3d/shapes/Shape3D.h"

class PhysicsBody3D
{
public:
    PhysicsBody3D(
        std::unique_ptr<Shape3D> shape,
        Vector3 position,
        Vector3 rotation = Vector3::zero(),
        Vector3 velocity = Vector3::zero(),
        float mass = 1.0f,
        PhysicsSurfaceMaterial3D surfaceMaterial = {},
        RigidBodySettings3D rigidBodySettings = {})
        : shape(std::move(shape)),
          position(position),
          rotation(rotation),
          velocity(velocity),
          acceleration(Vector3::zero()),
          mass(mass),
          inverseMass(mass > 0.0f ? 1.0f / mass : 0.0f),
          surfaceMaterial(surfaceMaterial),
          rigidBodySettings(rigidBodySettings),
          momentOfInertia(computeMomentOfInertia()),
          inverseMomentOfInertia(momentOfInertia > 0.0f ? 1.0f / momentOfInertia : 0.0f)
    {
    }

    Shape3D *getShape() { return shape.get(); }
    const Shape3D *getShape() const { return shape.get(); }

    const Vector3 &getPosition() const { return position; }
    void setPosition(const Vector3 &newPosition) { position = newPosition; }
    Vector3 getCenterOfMassWorldPosition() const
    {
        return position + orientation.toMatrix().transformVector(rigidBodySettings.centerOfMassOffset);
    }
    Vector3 getCenterOfMassOffsetWorld() const
    {
        return orientation.toMatrix().transformVector(rigidBodySettings.centerOfMassOffset);
    }

    const Vector3 &getRotation() const { return rotation; }
    void setRotation(const Vector3 &newRotation)
    {
        rotation = newRotation;
        orientation = Quaternion::fromEulerXYZ(rotation);
    }
    Matrix4 getRotationMatrix() const { return orientation.toMatrix(); }

    const Vector3 &getVelocity() const { return velocity; }
    void setVelocity(const Vector3 &newVelocity) { velocity = newVelocity; }

    const Vector3 &getAcceleration() const { return acceleration; }
    void setAcceleration(const Vector3 &newAcceleration) { acceleration = newAcceleration; }

    float getMass() const { return mass; }
    float getInverseMass() const { return inverseMass; }
    float getMomentOfInertia() const { return momentOfInertia; }
    float getInverseMomentOfInertia() const { return inverseMomentOfInertia; }
    const PhysicsSurfaceMaterial3D &getSurfaceMaterial() const { return surfaceMaterial; }
    PhysicsSurfaceMaterial3D &getSurfaceMaterial() { return surfaceMaterial; }
    const RigidBodySettings3D &getRigidBodySettings() const { return rigidBodySettings; }
    RigidBodySettings3D &getRigidBodySettings() { return rigidBodySettings; }
    const Vector3 &getAngularVelocity() const { return angularVelocity; }
    void setAngularVelocity(const Vector3 &newAngularVelocity) { angularVelocity = newAngularVelocity; }
    const Vector3 &getTorque() const { return torque; }
    void setTorque(const Vector3 &newTorque) { torque = newTorque; }

    virtual bool isStatic() const
    {
        return mass <= 0.0f;
    }

    virtual void applyForce(const Vector3 &force)
    {
        if (isStatic())
            return;

        acceleration += force * inverseMass;
    }

    virtual void applyForceAtPoint(const Vector3 &force, const Vector3 &point)
    {
        if (isStatic())
            return;

        applyForce(force);
        torque += (point - getCenterOfMassWorldPosition()).cross(force);
    }

    virtual void integrate(float dt)
    {
        if (isStatic())
            return;

        velocity += acceleration * dt;
        const float linearDampingFactor = std::max(0.0f, 1.0f - rigidBodySettings.linearDamping * dt);
        velocity *= linearDampingFactor;
        position += velocity * dt;
        angularVelocity += torque * inverseMomentOfInertia * dt;
        const float angularDampingFactor = std::max(0.0f, 1.0f - rigidBodySettings.angularDamping * dt);
        angularVelocity *= angularDampingFactor;
        const float angularSpeed = angularVelocity.length();
        if (angularSpeed > 0.0f)
        {
            orientation =
                (Quaternion::fromAxisAngle(angularVelocity / angularSpeed, angularSpeed * dt) * orientation).normalized();
        }
        rotation += angularVelocity * dt;
        acceleration = Vector3::zero();
        torque = Vector3::zero();
    }

    virtual ~PhysicsBody3D() = default;

private:
    float computeMomentOfInertia() const
    {
        if (isStatic())
        {
            return 0.0f;
        }

        if (const auto *sphere = dynamic_cast<const SphereShape3D *>(shape.get()))
        {
            const float radius = sphere->getRadius();
            return 0.4f * mass * radius * radius;
        }

        if (const auto *box = dynamic_cast<const BoxCollider3D *>(shape.get()))
        {
            const Vector3 e = box->getHalfExtents();
            const float ix = (1.0f / 3.0f) * mass * (e.y * e.y + e.z * e.z);
            const float iy = (1.0f / 3.0f) * mass * (e.x * e.x + e.z * e.z);
            const float iz = (1.0f / 3.0f) * mass * (e.x * e.x + e.y * e.y);
            return (ix + iy + iz) / 3.0f;
        }

        return 0.0f;
    }

    std::unique_ptr<Shape3D> shape;
    Vector3 position;
    Vector3 rotation;
    Vector3 velocity;
    Vector3 acceleration;
    float mass = 1.0f;
    float inverseMass = 0.0f;
    PhysicsSurfaceMaterial3D surfaceMaterial;
    RigidBodySettings3D rigidBodySettings;
    Vector3 angularVelocity = Vector3::zero();
    Vector3 torque = Vector3::zero();
    float momentOfInertia = 0.0f;
    float inverseMomentOfInertia = 0.0f;
    Quaternion orientation = Quaternion::identity();
};
