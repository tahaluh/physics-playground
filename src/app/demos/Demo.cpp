#include "app/demos/Demo.h"

#include <memory>

#include "app/objects/RingObject.h"
#include "app/objects/SphereObject.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"

namespace
{
const float kMoveSpeed = 4.0f;
const float kMouseLookSensitivity = 0.0025f;
const Vector3 kRingWorldOffset(-5.0f, 0.0f, 0.0f);
const Vector3 kSphereWorldOffset(5.0f, 0.0f, 0.0f);

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

Demo::~Demo() = default;

void Demo::onAttach(int viewportWidth, int viewportHeight)
{
    RingObjectDesc ringDesc = RingObject::makeDefaultDesc();
    ringDesc.worldOffset = kRingWorldOffset;
    ringObject = RingObject::create(ringDesc);

    SphereObjectDesc sphereDesc = SphereObject::makeDefaultDesc();
    sphereDesc.worldOffset = kSphereWorldOffset;
    sphereObject = SphereObject::create(sphereDesc);

    physicsWorld2D = std::make_unique<PhysicsWorld2D>();
    physicsWorld2D->setGravity(Vector2(0.0f, 980.0f));

    physicsWorld3D = std::make_unique<PhysicsWorld3D>();
    physicsWorld3D->setGravity(Vector3(0.0f, -7.5f, 0.0f));

    camera3D = std::make_unique<Camera3D>();
    camera3D->transform.position = Vector3(0.0f, 3.5f, 22.0f);
    camera3D->transform.rotation = Vector3(-0.12f, 0.0f, 0.0f);

    combinedScene = std::make_unique<Scene3D>();
    combinedScene->createDirectionalLight({
        Vector3(-0.4f, -1.0f, -0.25f),
        0xFFFFFFFF,
        0.55f});
    combinedScene->createDirectionalLight({
        Vector3(0.45f, -0.35f, 0.8f),
        0xFF9FD3FF,
        0.18f});
    combinedScene->createPointLight({
        kSphereWorldOffset + Vector3(0.0f, 0.5f, 0.0f),
        0xFFFFD6A5,
        0.75f,
        8.0f});
    rebuildCombinedScene();
    configureCamera(viewportWidth, viewportHeight);
}

void Demo::onResize(int viewportWidth, int viewportHeight)
{
    configureCamera(viewportWidth, viewportHeight);
}

void Demo::configureCamera(int viewportWidth, int viewportHeight)
{
    if (camera3D)
    {
        camera3D->setViewport(0, 0, viewportWidth, viewportHeight);
    }
}

void Demo::updateCamera(float dt)
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

void Demo::onFixedUpdate(float dt)
{
    updateCamera(dt);

    if (physicsWorld2D && ringObject)
    {
        ringObject->step(*physicsWorld2D, dt);
    }

    if (physicsWorld3D && sphereObject)
    {
        sphereObject->step(*physicsWorld3D, dt);
    }

    rebuildCombinedScene();
}

void Demo::onRender(IGraphicsDevice &graphicsDevice) const
{
    if (!camera3D || !combinedScene)
    {
        return;
    }

    graphicsDevice.renderScene3D(*camera3D, *combinedScene);
}

void Demo::rebuildCombinedScene()
{
    if (!combinedScene)
    {
        return;
    }

    combinedScene->clearEntities();

    if (sphereObject)
    {
        combinedScene->getAmbientLight() = sphereObject->getRenderScene().getAmbientLight();
    }
    else if (ringObject)
    {
        combinedScene->getAmbientLight() = ringObject->getRenderScene().getAmbientLight();
    }

    if (ringObject)
    {
        combinedScene->appendEntitiesFrom(ringObject->getRenderScene());
    }

    if (sphereObject)
    {
        combinedScene->appendEntitiesFrom(sphereObject->getRenderScene());
    }
}
