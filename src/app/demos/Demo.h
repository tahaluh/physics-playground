#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "engine/core/ApplicationLayer.h"
#include "engine/graphics/IGraphicsDevice.h"
#include "engine/math/Quaternion.h"
#include "engine/render/3d/Camera.h"
#include "engine/scene/3d/RuntimeScene.h"

class Demo : public ApplicationLayer
{
public:
    ~Demo() override;

    void onAttach(int viewportWidth, int viewportHeight) override;
    void onResize(int viewportWidth, int viewportHeight) override;
    void onUpdate(float dt) override;
    void onFixedUpdate(float dt) override;
    void onRender(IGraphicsDevice &graphicsDevice) const override;
    std::string getRuntimeStatusText() const override;

private:
    void updateCamera(float dt);
    void configureCamera(int viewportWidth, int viewportHeight);
    void updateDebugToggles();
    void resetScene();

    std::unique_ptr<Camera> camera3D;
    std::unique_ptr<RuntimeScene> runtimeScene;
    float cameraPitch = 0.0f;
    float cameraYaw = 0.0f;
    Quaternion initialCameraRotation = Quaternion::identity();
    bool showLightDebugMarkers = false;
    bool showWireframes = false;
    bool simulationPaused = false;
    std::unordered_map<const BodyObject *, int> collisionCounts;
    int viewportWidth = 0;
    int viewportHeight = 0;
};
