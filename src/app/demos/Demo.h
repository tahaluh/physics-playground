#pragma once

#include <memory>
#include <vector>

#include "engine/core/ApplicationLayer.h"
#include "engine/graphics/IGraphicsDevice.h"
#include "engine/physics/2d/PhysicsWorld2D.h"
#include "engine/physics/3d/PhysicsWorld3D.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/scene/3d/Scene3D.h"
#include "engine/scene/objects/ComposedObject3D.h"
#include "engine/scene/objects/RingObject.h"
#include "engine/scene/objects/SquareObject.h"
#include "engine/scene/objects/SphereArenaObject.h"
#include "engine/scene/objects/SphereObject.h"

class Demo : public ApplicationLayer
{
public:
    ~Demo() override;

    void onAttach(int viewportWidth, int viewportHeight) override;
    void onResize(int viewportWidth, int viewportHeight) override;
    void onUpdate(float dt) override;
    void onFixedUpdate(float dt) override;
    void onRender(IGraphicsDevice &graphicsDevice) const override;

private:
    void updateCamera(float dt);
    void configureCamera(int viewportWidth, int viewportHeight);
    void rebuildCombinedScene();
    void updateDebugToggles();

    std::unique_ptr<PhysicsWorld2D> physicsWorld2D;
    std::unique_ptr<PhysicsWorld3D> physicsWorld3D;
    std::unique_ptr<Camera3D> camera3D;
    std::unique_ptr<Scene3D> combinedScene;
    std::vector<std::unique_ptr<RingObject>> ringObjects;
    std::vector<std::unique_ptr<SquareObject>> squareObjects;
    std::vector<std::size_t> squareObjectRingIndices;
    std::vector<std::unique_ptr<SphereArenaObject>> sphereArenaObjects;
    std::vector<std::unique_ptr<SphereObject>> sphereObjects;
    std::vector<std::unique_ptr<ComposedObject3D>> composedObjects;
    bool showLightDebugMarkers = false;
    bool showWireframes = false;
    bool showPhysicsDebugMarkers = false;
    bool freezeFixedTicks = true;
};
