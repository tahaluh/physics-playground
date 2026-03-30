#pragma once

#include <memory>
#include <vector>

#include "engine/core/ApplicationLayer.h"
#include "engine/graphics/IGraphicsDevice.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/scene/3d/Scene3D.h"
#include "engine/scene/objects/BodyObject3D.h"

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
    struct SceneEntityRange
    {
        std::size_t start = 0;
        std::size_t count = 0;
    };

    void updateCamera(float dt);
    void configureCamera(int viewportWidth, int viewportHeight);
    void rebuildCombinedScene();
    void syncCombinedSceneEntities();
    void updateDebugToggles();
    void resetScene();
    void appendSceneAndRecordRange(const Scene3D &sourceScene, std::vector<SceneEntityRange> &ranges);
    void copySceneRange(const Scene3D &sourceScene, const SceneEntityRange &range);

    std::unique_ptr<Camera3D> camera3D;
    std::unique_ptr<Scene3D> combinedScene;
    std::vector<std::unique_ptr<BodyObject3D>> bodyObjects;
    std::vector<SceneEntityRange> bodyObjectRanges;
    bool showLightDebugMarkers = false;
    bool showWireframes = false;
    int viewportWidth = 0;
    int viewportHeight = 0;
};
