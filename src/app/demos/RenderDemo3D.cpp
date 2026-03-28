#include "app/demos/RenderDemo3D.h"

#include <memory>

#include "engine/graphics/IGraphicsDevice.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Vector3.h"
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/render/3d/Camera3D.h"

namespace
{
const float kMoveSpeed = 3.5f;
const float kMouseLookSensitivity = 0.0025f;
const float kMinimumCameraDistance = 2.5f;
const float kBallControlForce = 1000.0f;

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
    camera->transform.rotation = Vector3::zero();
    camera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    return camera;
}
}

RenderDemo3D::~RenderDemo3D() = default;

void RenderDemo3D::onAttach(int viewportWidth, int viewportHeight)
{
    camera = makeDefaultCamera(viewportWidth, viewportHeight);
    loadedScene = PhysicsRingScene::createDefault();
    physicsWorld = std::make_unique<PhysicsWorld2D>();

    physicsWorld->setGravity(Vector2(0.0f, 980.0f));
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
    {
        return;
    }

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
}

void RenderDemo3D::applyBallControl()
{
    if (!loadedScene)
    {
        return;
    }

    PhysicsBody2D *controlledBall = loadedScene->getControlledBody();
    if (!controlledBall)
    {
        return;
    }

    Vector2 force = Vector2::zero();
    if (Input::isKeyDown(EngineKeyCode::Left))
        force.x -= kBallControlForce;
    if (Input::isKeyDown(EngineKeyCode::Right))
        force.x += kBallControlForce;
    if (Input::isKeyDown(EngineKeyCode::Up))
        force.y -= kBallControlForce;
    if (Input::isKeyDown(EngineKeyCode::Down))
        force.y += kBallControlForce;

    if (force.lengthSquared() > 0.0f)
    {
        controlledBall->applyForce(force);
    }
}

void RenderDemo3D::onFixedUpdate(float dt)
{
    updateCamera(dt);
    applyBallControl();

    if (physicsWorld && loadedScene)
    {
        physicsWorld->step(loadedScene->getPhysicsScene(), dt);
        loadedScene->syncRenderScene();
    }
}

void RenderDemo3D::onRender(IGraphicsDevice &graphicsDevice) const
{
    if (!loadedScene || !camera)
    {
        return;
    }

    graphicsDevice.renderScene3D(*camera, loadedScene->getRenderScene());
}
