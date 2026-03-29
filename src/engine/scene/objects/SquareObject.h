#pragma once

#include <cstddef>
#include <memory>

#include "engine/math/Vector2.h"
#include "engine/physics/2d/Material2D.h"
#include "engine/render/3d/Material3D.h"

struct RingObjectDesc;
class Scene2D;
class Scene3D;

struct SquareObjectDesc
{
    std::size_t ringObjectIndex = 0;
    Vector2 startPosition = Vector2(400.0f, 300.0f);
    Vector2 startVelocity = Vector2::zero();
    bool gpuSimulated = true;
    float sizePixels = 44.0f;
    uint32_t color = 0xFFFF8A5B;
    PhysicsSurfaceMaterial2D physicsSurfaceMaterial = PhysicsSurfaceMaterial2D{0.9f, 0.03f, 0.012f};
    RigidBodySettings2D rigidBodySettings = RigidBodySettings2D{0.0f, 0.0f, true};
    Material3D material = Material3D{};
};

class SquareObject
{
public:
    ~SquareObject();

    static SquareObjectDesc makeDefaultDesc();
    static std::unique_ptr<SquareObject> create(const SquareObjectDesc &desc, Scene2D &physicsScene, const RingObjectDesc &ringConfig);
    void destroy();
    bool isValid() const;
    bool usesGpuSimulation() const;

    Scene3D &getRenderScene();
    const Scene3D &getRenderScene() const;
    void syncRenderScene(const Scene2D &physicsScene, const RingObjectDesc &ringConfig);

private:
    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::unique_ptr<Scene3D> renderScene;
    std::size_t bodyIndex = kInvalidIndex;
    std::size_t squareEntityIndex = kInvalidIndex;
    SquareObjectDesc config;
};
