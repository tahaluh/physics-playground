#pragma once

#include "engine/render/3d/SolidRenderer3D.h"
#include "engine/render/3d/WireframeRenderer3D.h"

class Camera3D;
class Renderer2D;
class Scene3D;

class SceneRenderer3D
{
public:
    void render(Renderer2D &renderer, const Camera3D &camera, const Scene3D &scene) const;

private:
    SolidRenderer3D solidRenderer;
    WireframeRenderer3D wireframeRenderer;
};
