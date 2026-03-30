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

struct RingDynamicBodyVisualBinding
{
    std::size_t bodyIndex = static_cast<std::size_t>(-1);
    std::size_t entityIndex = static_cast<std::size_t>(-1);
    std::size_t rotationIndicatorEntityIndex = static_cast<std::size_t>(-1);
    Vector3 scale = Vector3::one();
    Vector3 rotationIndicatorScale = Vector3::one();
    float rotationIndicatorOffset = 0.0f;
    bool usesCustomScale = false;
};

struct RingObjectDesc
{
    Vector3 worldOffset = Vector3::zero();
    Vector2 center = Vector2(400.0f, 300.0f);
    Vector2 ballStartPosition = Vector2(230.0f, 180.0f);
    Vector2 ballStartVelocity = Vector2(110.0f, -40.0f);
    float simulationScale = 100.0f;
    float borderRadiusPixels = 200.0f;
    float borderThicknessPixels = 12.0f;
    float ballRadiusPixels = 10.0f;
    float ballOutlineThicknessWorld = 0.02f;
    float planeThicknessWorld = 0.04f;
    float planeZ = 0.0f;
    int borderSegments = 96;
    int ballSegments = 48;
    bool showRotationIndicators = true;
    uint32_t rotationIndicatorColor = 0xFFFFD166;
    uint32_t borderColor = 0xFFFFFFFF;
    uint32_t ballColor = 0xFFFFFFFF;
    PhysicsSurfaceMaterial2D borderSurfaceMaterial = PhysicsSurfaceMaterial2D{0.2f, 0.45f, 0.25f};
    RigidBodySettings2D borderRigidBodySettings = RigidBodySettings2D{0.0f, 0.0f, false};
    PhysicsSurfaceMaterial2D ballSurfaceMaterial = PhysicsSurfaceMaterial2D{0.2f, 0.68f, 0.6f};
    RigidBodySettings2D ballRigidBodySettings = RigidBodySettings2D{0.01f, 0.015f, true};
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
    const RingObjectDesc &getConfig() const;
    PhysicsBody2D *getControlledBody();
    const PhysicsBody2D *getControlledBody() const;
    void setWorldOffset(const Vector3 &offset);
    const Vector3 &getWorldOffset() const;
    void step(PhysicsWorld2D &physicsWorld, float dt);
    void syncRenderScene();
    void appendDebugMarkers(Scene3D &targetScene) const;

private:
    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::unique_ptr<Scene2D> physicsScene;
    std::unique_ptr<Scene3D> renderScene;
    std::size_t controlledBodyIndex = kInvalidIndex;
    std::size_t borderBodyIndex = kInvalidIndex;
    std::size_t borderEntityIndex = kInvalidIndex;
    std::vector<RingDynamicBodyVisualBinding> dynamicBindings;
    Vector3 worldOffset = Vector3::zero();
    RingObjectDesc config;
};
