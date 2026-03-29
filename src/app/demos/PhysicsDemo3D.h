#pragma once

#include <memory>

#include "app/scenes/PhysicsSphereScene3D.h"
#include "engine/core/ApplicationLayer.h"
#include "engine/graphics/IGraphicsDevice.h"
#include "engine/physics/3d/PhysicsWorld3D.h"
#include "engine/render/3d/Camera3D.h"

class PhysicsDemo3D : public ApplicationLayer
{
public:
    ~PhysicsDemo3D() override;

    void onAttach(int viewportWidth, int viewportHeight) override;
    void onResize(int viewportWidth, int viewportHeight) override;
    void onFixedUpdate(float dt) override;
    void onRender(IGraphicsDevice &graphicsDevice) const override;

private:
    void updateCamera(float dt);

    std::unique_ptr<Camera3D> camera;
    std::unique_ptr<PhysicsSphereScene3D> loadedScene;
    std::unique_ptr<PhysicsWorld3D> physicsWorld;
};
