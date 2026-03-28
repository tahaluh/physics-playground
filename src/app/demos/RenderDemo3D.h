#pragma once

#include <memory>

#include "engine/core/ApplicationLayer.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/render/3d/SceneRenderer3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/Scene3D.h"

class Renderer2D;

class RenderDemo3D : public ApplicationLayer
{
public:
    ~RenderDemo3D() override;

    void onAttach(int viewportWidth, int viewportHeight) override;
    void onResize(int viewportWidth, int viewportHeight) override;
    void onFixedUpdate(float dt) override;
    void onRender(Renderer2D &renderer) const override;

private:
    void updateCamera(float dt);

    std::unique_ptr<Camera3D> camera;
    std::unique_ptr<SceneRenderer3D> sceneRenderer;
    std::unique_ptr<Scene3D> scene;
    Entity3D *cubeEntity = nullptr;
    float cubeRotation = 0.0f;
};
