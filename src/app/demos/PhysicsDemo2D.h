#pragma once

#include <memory>

#include "engine/core/ApplicationLayer.h"
#include "engine/physics/2d/PhysicsWorld2D.h"
#include "engine/scene/2d/Scene2D.h"

class IGraphicsDevice;

class PhysicsDemo2D : public ApplicationLayer
{
public:
    ~PhysicsDemo2D() override;

    void onAttach(int viewportWidth, int viewportHeight) override;
    void onResize(int viewportWidth, int viewportHeight) override;
    void onFixedUpdate(float dt) override;
    void onRender(IGraphicsDevice &graphicsDevice) const override;

private:
    std::unique_ptr<Scene2D> scene;
    std::unique_ptr<PhysicsWorld2D> physicsWorld;
};
