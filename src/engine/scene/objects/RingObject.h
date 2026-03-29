#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"
#include "engine/physics/2d/Material2D.h"
#include "engine/render/3d/Material3D.h"

class PhysicsBody2D;
class PhysicsWorld2D;
class Scene2D;
class Scene3D;

struct RingObjectDesc
{
    Vector3 worldOffset = Vector3::zero();
    Vector2 center = Vector2(400.0f, 300.0f);
    Vector2 ballStartPosition = Vector2(230.0f, 180.0f);
    Vector2 ballStartVelocity = Vector2(110.0f, -40.0f);
    float simulationScale = 100.0f;
    float borderRadiusPixels = 200.0f;
    float borderThicknessWorld = 0.02f;
    float ballRadiusPixels = 10.0f;
    float ballOutlineThicknessWorld = 0.02f;
    float planeThicknessWorld = 0.04f;
    float planeZ = 0.0f;
    int borderSegments = 96;
    int ballSegments = 48;
    uint32_t borderColor = 0xFFFFFFFF;
    uint32_t ballColor = 0xFFFFFFFF;
    PhysicsSurfaceMaterial2D physicsSurfaceMaterial = PhysicsSurfaceMaterial2D{0.9f, 0.03f, 0.012f};
    RigidBodySettings2D rigidBodySettings = RigidBodySettings2D{0.0f, 0.0f, true};
    Material3D borderMaterial = Material3D{};
    Material3D ballMaterial = Material3D{};
};

class RingObject
{
public:
    ~RingObject();

    static RingObjectDesc makeDefaultDesc();
    static std::unique_ptr<RingObject> create(const RingObjectDesc &desc);
    static std::unique_ptr<RingObject> createDefault();
    void destroy();
    bool isValid() const;

    Scene2D &getPhysicsScene();
    const Scene2D &getPhysicsScene() const;
    Scene3D &getRenderScene();
    const Scene3D &getRenderScene() const;
    PhysicsBody2D *getControlledBody();
    const PhysicsBody2D *getControlledBody() const;
    void setWorldOffset(const Vector3 &offset);
    const Vector3 &getWorldOffset() const;
    void step(PhysicsWorld2D &physicsWorld, float dt);
    void syncRenderScene();

private:
    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::unique_ptr<Scene2D> physicsScene;
    std::unique_ptr<Scene3D> renderScene;
    std::size_t controlledBodyIndex = kInvalidIndex;
    std::size_t borderBodyIndex = kInvalidIndex;
    std::size_t borderEntityIndex = kInvalidIndex;
    std::vector<std::size_t> dynamicBodyIndices;
    std::vector<std::size_t> dynamicEntityIndices;
    Vector3 worldOffset = Vector3::zero();
    RingObjectDesc config;
};
