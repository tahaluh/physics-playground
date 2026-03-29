#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "engine/core/ApplicationLayer.h"
#include "engine/math/Vector3.h"
#include "engine/graphics/IGraphicsDevice.h"
#include "engine/physics/2d/PhysicsWorld2D.h"
#include "engine/physics/3d/PhysicsWorld3D.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/scene/2d/Scene2D.h"
#include "engine/scene/3d/PhysicsScene3D.h"
#include "engine/scene/3d/Scene3D.h"

class Demo : public ApplicationLayer
{
public:
    ~Demo() override;

    void onAttach(int viewportWidth, int viewportHeight) override;
    void onResize(int viewportWidth, int viewportHeight) override;
    void onFixedUpdate(float dt) override;
    void onRender(IGraphicsDevice &graphicsDevice) const override;

private:
    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    void updateCamera(float dt);
    void configureCamera(int viewportWidth, int viewportHeight);
    void rebuildCombinedScene();

    void createRingObject();
    void createSphereObject();
    void syncRingRenderScene();
    void syncSphereRenderScene();

    std::unique_ptr<PhysicsWorld2D> physicsWorld2D;
    std::unique_ptr<PhysicsWorld3D> physicsWorld3D;
    std::unique_ptr<Camera3D> camera3D;
    std::unique_ptr<Scene3D> combinedScene;

    std::unique_ptr<Scene2D> ringPhysicsScene;
    std::unique_ptr<Scene3D> ringRenderScene;
    std::size_t ringControlledBodyIndex = kInvalidIndex;
    std::size_t ringBorderBodyIndex = kInvalidIndex;
    std::size_t ringBorderEntityIndex = kInvalidIndex;
    std::vector<std::size_t> ringDynamicBodyIndices;
    std::vector<std::size_t> ringDynamicEntityIndices;
    Vector3 ringWorldOffset = Vector3::zero();

    std::unique_ptr<PhysicsScene3D> spherePhysicsScene;
    std::unique_ptr<Scene3D> sphereRenderScene;
    std::size_t sphereBorderBodyIndex = kInvalidIndex;
    std::size_t sphereBallBodyIndex = kInvalidIndex;
    std::size_t sphereBorderEntityIndex = kInvalidIndex;
    std::size_t sphereBallEntityIndex = kInvalidIndex;
    Vector3 sphereWorldOffset = Vector3::zero();
};
