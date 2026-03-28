#include "app/demos/RenderDemo3D.h"

#include <memory>

#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Vector3.h"
#include "engine/render/2d/Renderer2D.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/render/3d/SceneRenderer3D.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
constexpr float kCubeSize = 1.8f;
constexpr float kMoveSpeed = 3.5f;
constexpr float kTurnSpeed = 1.5f;
constexpr float kMouseLookSensitivity = 0.0025f;
constexpr float kMinimumCameraDistance = 2.6f;

Vector3 getPlanarForward(const Camera3D &camera)
{
    Vector3 forward = camera.getForward();
    forward.y = 0.0f;
    if (forward.lengthSquared() == 0.0f)
    {
        return Vector3(0.0f, 0.0f, -1.0f);
    }
    return forward.normalized();
}

Vector3 getPlanarRight(const Camera3D &camera)
{
    return Vector3::up().cross(getPlanarForward(camera) * -1.0f).normalized();
}

std::unique_ptr<Camera3D> makeDefaultCamera(int width, int height)
{
    auto camera = std::make_unique<Camera3D>();
    camera->transform.position = Vector3(0.0f, 0.0f, 6.0f);
    camera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    return camera;
}
}

RenderDemo3D::~RenderDemo3D() = default;

void RenderDemo3D::onAttach(int viewportWidth, int viewportHeight)
{
    camera = makeDefaultCamera(viewportWidth, viewportHeight);
    sceneRenderer = std::make_unique<SceneRenderer3D>();
    scene = std::make_unique<Scene3D>();

    Entity3D cube;
    cube.name = "MainCube";
    cube.mesh = MeshFactory3D::makeCube(kCubeSize);
    cube.material.fillColor = 0xFF3A86FF;
    cube.material.wireframeColor = 0xFF44AAFF;
    cube.material.renderSolid = true;
    cube.material.renderWireframe = false;
    cubeEntity = &scene->createEntity(cube);
}

void RenderDemo3D::onResize(int viewportWidth, int viewportHeight)
{
    if (!camera || viewportWidth <= 0 || viewportHeight <= 0)
    {
        return;
    }

    camera->setAspectRatio(static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight));
}

void RenderDemo3D::updateCamera(float dt)
{
    if (!camera)
        return;

    const Vector3 planarForward = getPlanarForward(*camera);
    const Vector3 planarRight = getPlanarRight(*camera);
    Vector3 movement = Vector3::zero();
    if (Input::isKeyDown(EngineKeyCode::W))
        movement += planarForward;
    if (Input::isKeyDown(EngineKeyCode::S))
        movement -= planarForward;
    if (Input::isKeyDown(EngineKeyCode::D))
        movement += planarRight;
    if (Input::isKeyDown(EngineKeyCode::A))
        movement -= planarRight;
    if (Input::isKeyDown(EngineKeyCode::E))
        movement += Vector3::up();
    if (Input::isKeyDown(EngineKeyCode::Q))
        movement -= Vector3::up();

    if (movement.lengthSquared() > 0.0f)
    {
        camera->transform.position += movement.normalized() * (kMoveSpeed * dt);
    }

    const float distanceFromCenter = camera->transform.position.length();
    if (distanceFromCenter < kMinimumCameraDistance)
    {
        if (distanceFromCenter == 0.0f)
        {
            camera->transform.position = Vector3(0.0f, 0.0f, kMinimumCameraDistance);
        }
        else
        {
            camera->transform.position = camera->transform.position.normalized() * kMinimumCameraDistance;
        }
    }

    camera->transform.rotation.y -= Input::getMouseDeltaX() * kMouseLookSensitivity;
    camera->transform.rotation.x -= Input::getMouseDeltaY() * kMouseLookSensitivity;

    const float maxPitch = 1.4f;
    camera->transform.rotation.x = Vector3::clamp(camera->transform.rotation.x, -maxPitch, maxPitch);

    if (Input::isKeyDown(EngineKeyCode::Left))
        camera->transform.rotation.y += kTurnSpeed * dt;
    if (Input::isKeyDown(EngineKeyCode::Right))
        camera->transform.rotation.y -= kTurnSpeed * dt;
    if (Input::isKeyDown(EngineKeyCode::Up))
        camera->transform.rotation.x += kTurnSpeed * dt;
    if (Input::isKeyDown(EngineKeyCode::Down))
        camera->transform.rotation.x -= kTurnSpeed * dt;
}

void RenderDemo3D::onFixedUpdate(float dt)
{
    updateCamera(dt);

    if (!cubeEntity)
        return;

    cubeRotation += dt;
    cubeEntity->transform.position = Vector3::zero();
    cubeEntity->transform.rotation = Vector3(cubeRotation * 0.6f, cubeRotation, cubeRotation * 0.3f);
    cubeEntity->transform.scale = Vector3::one();
}

void RenderDemo3D::onRender(Renderer2D &renderer) const
{
    if (!scene || !camera)
        return;

    sceneRenderer->render(renderer, *camera, *scene);
}
