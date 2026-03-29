#include "app/demos/PhysicsComparisonDemo.h"

#include <memory>

#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Vector3.h"

namespace
{
const float kMoveSpeed = 4.0f;
const float kMouseLookSensitivity = 0.0025f;
const Vector3 kScene2DOffset(-5.0f, 0.0f, 0.0f);
const Vector3 kScene3DOffset(5.0f, 0.0f, 0.0f);

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
}

PhysicsComparisonDemo::~PhysicsComparisonDemo() = default;

void PhysicsComparisonDemo::onAttach(int viewportWidth, int viewportHeight)
{
    scene2D = PhysicsRingScene::createDefault();
    physicsWorld2D = std::make_unique<PhysicsWorld2D>();
    physicsWorld2D->setGravity(Vector2(0.0f, 980.0f));
    scene2D->setWorldOffset(kScene2DOffset);

    camera3D = std::make_unique<Camera3D>();
    camera3D->transform.position = Vector3(0.0f, 3.5f, 22.0f);
    camera3D->transform.rotation = Vector3(-0.12f, 0.0f, 0.0f);
    scene3D = PhysicsSphereScene3D::createDefault();
    scene3D->setWorldOffset(kScene3DOffset);
    physicsWorld3D = std::make_unique<PhysicsWorld3D>();
    physicsWorld3D->setGravity(Vector3(0.0f, -7.5f, 0.0f));

    configureCamera(viewportWidth, viewportHeight);
}

void PhysicsComparisonDemo::configureCamera(int viewportWidth, int viewportHeight)
{
    if (camera3D)
    {
        camera3D->setViewport(0, 0, viewportWidth, viewportHeight);
    }
}

void PhysicsComparisonDemo::onResize(int viewportWidth, int viewportHeight)
{
    configureCamera(viewportWidth, viewportHeight);
}

void PhysicsComparisonDemo::updateCamera(float dt)
{
    if (!camera3D)
    {
        return;
    }

    const Vector3 planarForward = getPlanarForward(*camera3D);
    const Vector3 planarRight = getPlanarRight(*camera3D);
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
        camera3D->transform.position += movement.normalized() * (kMoveSpeed * dt);
    }

    camera3D->transform.rotation.y -= Input::getMouseDeltaX() * kMouseLookSensitivity;
    camera3D->transform.rotation.x -= Input::getMouseDeltaY() * kMouseLookSensitivity;
    camera3D->transform.rotation.x = Vector3::clamp(camera3D->transform.rotation.x, -1.4f, 1.4f);
}

void PhysicsComparisonDemo::onFixedUpdate(float dt)
{
    updateCamera(dt);

    if (physicsWorld2D && scene2D)
    {
        physicsWorld2D->step(scene2D->getPhysicsScene(), dt);
        scene2D->syncRenderScene();
    }

    if (physicsWorld3D && scene3D)
    {
        physicsWorld3D->step(scene3D->getPhysicsScene(), dt);
        scene3D->syncRenderScene();
    }
}

void PhysicsComparisonDemo::onRender(IGraphicsDevice &graphicsDevice) const
{
    if (!camera3D)
    {
        return;
    }

    if (scene2D)
    {
        graphicsDevice.renderScene3D(*camera3D, scene2D->getRenderScene());
    }

    if (scene3D)
    {
        graphicsDevice.renderScene3D(*camera3D, scene3D->getRenderScene());
    }
}
