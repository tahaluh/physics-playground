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

struct SphereArenaObjectDesc
{
    Vector3 worldOffset = Vector3::zero();
    float boundaryRadius = 4.0f;
    int boundarySphereRings = 20;
    int boundarySphereSegments = 32;
    Material3D borderMaterial = Material3D{};

    float ballRadius = 0.45f;
    Vector3 ballStartPosition = Vector3(0.9f, 1.4f, -0.6f);
    Vector3 ballStartVelocity = Vector3(1.8f, -0.4f, 1.2f);
    int ballSphereRings = 16;
    int ballSphereSegments = 24;
    PhysicsSurfaceMaterial3D ballSurfaceMaterial = PhysicsSurfaceMaterial3D{0.75f, 0.5f};
    RigidBodySettings3D ballRigidBodySettings = RigidBodySettings3D{0.03f, 0.04f, true};
    Material3D ballMaterial = Material3D{};

    bool enableCubeBody = true;
    float cubeSize = 0.9f;
    float cubeColliderRadius = 0.45f;
    Vector3 cubeStartPosition = Vector3(-1.0f, 0.6f, 0.4f);
    Vector3 cubeStartVelocity = Vector3(-1.2f, 0.1f, -1.0f);
    PhysicsSurfaceMaterial3D cubeSurfaceMaterial = PhysicsSurfaceMaterial3D{0.22f, 0.72f};
    RigidBodySettings3D cubeRigidBodySettings = RigidBodySettings3D{0.18f, 0.65f, true, Vector3(0.0f, -0.08f, 0.0f)};
    Material3D cubeMaterial = Material3D{};
};

class SphereArenaObject
{
public:
    ~SphereArenaObject();

    static SphereArenaObjectDesc makeDefaultDesc();
    static std::unique_ptr<SphereArenaObject> create(const SphereArenaObjectDesc &desc);
    static std::unique_ptr<SphereArenaObject> createDefault();
    void destroy();
    bool isValid() const;

    PhysicsScene3D &getPhysicsScene();
    const PhysicsScene3D &getPhysicsScene() const;
    Scene3D &getRenderScene();
    const Scene3D &getRenderScene() const;
    PhysicsBody3D *getBallBody();
    const PhysicsBody3D *getBallBody() const;
    PhysicsBody3D *getCubeBody();
    const PhysicsBody3D *getCubeBody() const;
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
    std::size_t cubeBodyIndex = kInvalidIndex;
    std::size_t borderEntityIndex = kInvalidIndex;
    std::size_t ballEntityIndex = kInvalidIndex;
    std::size_t cubeEntityIndex = kInvalidIndex;
    Vector3 worldOffset = Vector3::zero();
    SphereArenaObjectDesc config;
};
