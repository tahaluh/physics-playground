#include "app/demos/Demo.h"

#include <memory>

#include "app/objects/RingObject.h"
#include "app/objects/SphereObject.h"
#include "engine/input/InputAction.h"
#include "engine/input/Input.h"
#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Entity3D.h"

namespace
{
const float kMoveSpeed = 4.0f;
const float kMouseLookSensitivity = 0.0025f;
const Vector3 kRingWorldOffset(-5.0f, 0.0f, 0.0f);
const Vector3 kSphereWorldOffset(5.0f, 0.0f, 0.0f);
const Vector3 kPointLightPosition = kSphereWorldOffset + Vector3(2.8f, 0.5f, 0.0f);

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
    sphereDesc.ballMaterial.solid.color = 0xFFFFFFFF;
    sphereDesc.ballMaterial.wireframe.color = 0xFF707070;
    sphereDesc.ballMaterial.solid.emissiveColor = 0x00000000;
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
    combinedScene->createPointLight({
        kPointLightPosition,
        0xFFFF4040,
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
    if (Input::isActionDown(EngineInputAction::MoveForward))
        movement += planarForward;
    if (Input::isActionDown(EngineInputAction::MoveBackward))
        movement -= planarForward;
    if (Input::isActionDown(EngineInputAction::MoveRight))
        movement += planarRight;
    if (Input::isActionDown(EngineInputAction::MoveLeft))
        movement -= planarRight;
    if (Input::isActionDown(EngineInputAction::MoveUp))
        movement += Vector3::up();
    if (Input::isActionDown(EngineInputAction::MoveDown))
        movement -= Vector3::up();

    if (movement.lengthSquared() > 0.0f)
    {
        camera3D->transform.position += movement.normalized() * (kMoveSpeed * dt);
    }

    camera3D->transform.rotation.y -= Input::getMouseDeltaX() * kMouseLookSensitivity;
    camera3D->transform.rotation.x -= Input::getMouseDeltaY() * kMouseLookSensitivity;
    camera3D->transform.rotation.x = Vector3::clamp(camera3D->transform.rotation.x, -1.4f, 1.4f);
}

void Demo::updateDebugToggles()
{
    bool changed = false;

    if (Input::wasActionPressed(EngineInputAction::ToggleLightDebug))
    {
        showLightDebugMarkers = !showLightDebugMarkers;
        changed = true;
    }

    if (Input::wasActionPressed(EngineInputAction::ToggleWireframe))
    {
        showWireframes = !showWireframes;
        changed = true;
    }

    if (changed)
    {
        rebuildCombinedScene();
    }
}

void Demo::onFixedUpdate(float dt)
{
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

void Demo::onUpdate(float dt)
{
    updateDebugToggles();
    updateCamera(dt);
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

    if (!showWireframes)
    {
        for (Entity3D &entity : combinedScene->getEntities())
        {
            entity.material.renderWireframe = false;
        }
    }

    if (showLightDebugMarkers)
    {
        Entity3D pointLightMarker;
        pointLightMarker.name = "PointLightMarker";
        pointLightMarker.transform.position = kPointLightPosition;
        pointLightMarker.mesh = MeshFactory3D::makeSphere(0.16f, 10, 16, 0);
        pointLightMarker.material.solid.color = 0xFF000000;
        pointLightMarker.material.solid.emissiveColor = 0xFFFF4040;
        pointLightMarker.material.solid.ambientFactor = 0.0f;
        pointLightMarker.material.solid.diffuseFactor = 0.0f;
        pointLightMarker.material.wireframe.color = 0xFF000000;
        pointLightMarker.material.wireframe.emissiveColor = 0xFFFF4040;
        pointLightMarker.material.wireframe.ambientFactor = 0.0f;
        pointLightMarker.material.renderSolid = true;
        pointLightMarker.material.renderWireframe = false;
        combinedScene->createEntity(pointLightMarker);
    }
}
