#pragma once

#include <memory>

#include "app/scenes/PhysicsRingScene.h"
#include "engine/core/ApplicationLayer.h"
#include "engine/graphics/IGraphicsDevice.h"
#include "engine/physics/2d/PhysicsWorld2D.h"
#include "engine/render/3d/Camera3D.h"

class RenderDemo3D : public ApplicationLayer
{
public:
    ~RenderDemo3D() override;

    void onAttach(int viewportWidth, int viewportHeight) override;
    void onResize(int viewportWidth, int viewportHeight) override;
    void onFixedUpdate(float dt) override;
    void onRender(IGraphicsDevice &graphicsDevice) const override;

private:
    void updateCamera(float dt);
    void applyBallControl();

    std::unique_ptr<Camera3D> camera;
    std::unique_ptr<PhysicsRingScene> loadedScene;
    std::unique_ptr<PhysicsWorld2D> physicsWorld;
};
