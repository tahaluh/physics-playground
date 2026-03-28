#include "app/demos/RenderDemo3D.h"

#include "engine/math/Transform3D.h"
#include "engine/math/Vector3.h"
#include "engine/render/2d/Renderer2D.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/render/3d/SolidRenderer3D.h"
#include "engine/render/3d/WireframeRenderer3D.h"

namespace
{
constexpr float kCubeSize = 1.8f;

Camera3D *makeDefaultCamera(int width, int height)
{
    Camera3D *camera = new Camera3D();
    camera->transform.position = Vector3(0.0f, 0.0f, 6.0f);
    camera->aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    return camera;
}
}

void RenderDemo3D::initialize(int viewportWidth, int viewportHeight)
{
    if (!camera)
        camera = makeDefaultCamera(viewportWidth, viewportHeight);
    if (!solidRenderer)
        solidRenderer = new SolidRenderer3D();
    if (!wireframeRenderer)
        wireframeRenderer = new WireframeRenderer3D();
    if (!cubeTransform)
        cubeTransform = new Transform3D();
}

void RenderDemo3D::resizeViewport(int viewportWidth, int viewportHeight)
{
    if (!camera || viewportWidth <= 0 || viewportHeight <= 0)
    {
        return;
    }

    camera->aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
}

void RenderDemo3D::step(float dt)
{
    cubeRotation += dt;
    cubeTransform->position = Vector3(0.0f, 0.0f, 0.0f);
    cubeTransform->rotation = Vector3(cubeRotation * 0.6f, cubeRotation, cubeRotation * 0.3f);
    cubeTransform->scale = Vector3::one();
}

void RenderDemo3D::render(Renderer2D &renderer) const
{
    solidRenderer->drawCube(renderer, *camera, *cubeTransform, kCubeSize);
    wireframeRenderer->drawCube(renderer, *camera, *cubeTransform, kCubeSize, 0xFF44AAFF);
}
