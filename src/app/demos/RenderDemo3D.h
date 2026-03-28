#pragma once

#include <memory>
#include <vector>

#include "engine/core/ApplicationLayer.h"
#include "engine/graphics/IGraphicsDevice.h"
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/PhysicsWorld2D.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/scene/2d/Scene2D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/Scene3D.h"

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
    void syncSceneFromPhysics();

    std::unique_ptr<Camera3D> camera;
    std::unique_ptr<Scene2D> physicsScene;
    std::unique_ptr<PhysicsWorld2D> physicsWorld;
    std::unique_ptr<Scene3D> scene;
    PhysicsBody2D *controlledBall = nullptr;
    Entity3D *borderEntity = nullptr;
    std::vector<Entity3D *> ballEntities;
};
