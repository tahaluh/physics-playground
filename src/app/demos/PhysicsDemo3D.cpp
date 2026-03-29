#include "app/demos/PhysicsDemo3D.h"

#include <memory>

#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Vector3.h"

namespace
{
const float kMoveSpeed = 4.0f;
const float kMouseLookSensitivity = 0.0025f;
const float kMinimumCameraDistance = 6.0f;

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
    camera->transform.position = Vector3(0.0f, 1.2f, 9.5f);
    camera->transform.rotation = Vector3(0.0f, 3.14159265f, 0.0f);
    camera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    return camera;
}
}

PhysicsDemo3D::~PhysicsDemo3D() = default;

void PhysicsDemo3D::onAttach(int viewportWidth, int viewportHeight)
{
    camera = makeDefaultCamera(viewportWidth, viewportHeight);
    loadedScene = PhysicsSphereScene3D::createDefault();
    physicsWorld = std::make_unique<PhysicsWorld3D>();
    physicsWorld->setGravity(Vector3(0.0f, -7.5f, 0.0f));
}

void PhysicsDemo3D::onResize(int viewportWidth, int viewportHeight)
{
    if (!camera || viewportWidth <= 0 || viewportHeight <= 0)
    {
        return;
    }

    camera->setAspectRatio(static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight));
}

void PhysicsDemo3D::updateCamera(float dt)
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

    const float distanceFromOrigin = camera->transform.position.length();
    if (distanceFromOrigin < kMinimumCameraDistance && distanceFromOrigin > 0.0f)
    {
        camera->transform.position = camera->transform.position.normalized() * kMinimumCameraDistance;
    }

    camera->transform.rotation.y -= Input::getMouseDeltaX() * kMouseLookSensitivity;
    camera->transform.rotation.x -= Input::getMouseDeltaY() * kMouseLookSensitivity;
    camera->transform.rotation.x = Vector3::clamp(camera->transform.rotation.x, -1.4f, 1.4f);
}

void PhysicsDemo3D::onFixedUpdate(float dt)
{
    updateCamera(dt);

    if (physicsWorld && loadedScene)
    {
        physicsWorld->step(loadedScene->getPhysicsScene(), dt);
        loadedScene->syncRenderScene();
    }
}

void PhysicsDemo3D::onRender(IGraphicsDevice &graphicsDevice) const
{
    if (!camera || !loadedScene)
    {
        return;
    }

    graphicsDevice.renderScene3D(*camera, loadedScene->getRenderScene());
}
