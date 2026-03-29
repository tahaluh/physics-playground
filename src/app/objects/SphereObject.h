#pragma once

#include <cstddef>
#include <memory>

#include "engine/math/Vector3.h"
#include "engine/physics/3d/PhysicsMaterial3D.h"
#include "engine/render/3d/Material3D.h"

class PhysicsBody3D;
class PhysicsScene3D;
class PhysicsWorld3D;
class Scene3D;

struct SphereObjectDesc
{
    Vector3 worldOffset = Vector3::zero();
    float boundaryRadius = 4.0f;
    float ballRadius = 0.45f;
    Vector3 ballStartPosition = Vector3(0.9f, 1.4f, -0.6f);
    Vector3 ballStartVelocity = Vector3(1.8f, -0.4f, 1.2f);
    PhysicsMaterial3D physicsMaterial = PhysicsMaterial3D{0.03f, 0.75f, 0.5f, true};
    int boundarySphereRings = 20;
    int boundarySphereSegments = 32;
    int ballSphereRings = 16;
    int ballSphereSegments = 24;
    Material3D borderMaterial = Material3D{};
    Material3D ballMaterial = Material3D{};
};

class SphereObject
{
public:
    ~SphereObject();

    static SphereObjectDesc makeDefaultDesc();
    static std::unique_ptr<SphereObject> create(const SphereObjectDesc &desc);
    static std::unique_ptr<SphereObject> createDefault();
    void destroy();
    bool isValid() const;

    PhysicsScene3D &getPhysicsScene();
    const PhysicsScene3D &getPhysicsScene() const;
    Scene3D &getRenderScene();
    const Scene3D &getRenderScene() const;
    PhysicsBody3D *getBallBody();
    const PhysicsBody3D *getBallBody() const;
    void setWorldOffset(const Vector3 &offset);
    const Vector3 &getWorldOffset() const;
    void step(PhysicsWorld3D &physicsWorld, float dt);
    void syncRenderScene();

private:
    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::unique_ptr<PhysicsScene3D> physicsScene;
    std::unique_ptr<Scene3D> renderScene;
    std::size_t borderBodyIndex = kInvalidIndex;
    std::size_t ballBodyIndex = kInvalidIndex;
    std::size_t borderEntityIndex = kInvalidIndex;
    std::size_t ballEntityIndex = kInvalidIndex;
    Vector3 worldOffset = Vector3::zero();
    SphereObjectDesc config;
};
