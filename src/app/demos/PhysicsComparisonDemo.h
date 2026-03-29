#pragma once

#include <memory>

#include "app/scenes/PhysicsRingScene.h"
#include "app/scenes/PhysicsSphereScene3D.h"
#include "engine/core/ApplicationLayer.h"
#include "engine/graphics/IGraphicsDevice.h"
#include "engine/physics/2d/PhysicsWorld2D.h"
#include "engine/physics/3d/PhysicsWorld3D.h"
#include "engine/render/3d/Camera3D.h"

class PhysicsComparisonDemo : public ApplicationLayer
{
public:
    ~PhysicsComparisonDemo() override;

    void onAttach(int viewportWidth, int viewportHeight) override;
    void onResize(int viewportWidth, int viewportHeight) override;
    void onFixedUpdate(float dt) override;
    void onRender(IGraphicsDevice &graphicsDevice) const override;

private:
    void updateCamera(float dt);
    void configureCamera(int viewportWidth, int viewportHeight);

    std::unique_ptr<PhysicsRingScene> scene2D;
    std::unique_ptr<PhysicsWorld2D> physicsWorld2D;
    std::unique_ptr<Camera3D> camera3D;
    std::unique_ptr<PhysicsSphereScene3D> scene3D;
    std::unique_ptr<PhysicsWorld3D> physicsWorld3D;
};
