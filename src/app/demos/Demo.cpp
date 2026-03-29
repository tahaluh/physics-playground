#include "app/demos/Demo.h"

#include <memory>

#include "app/objects/RingObject.h"
#include "app/objects/SphereObject.h"
#include "engine/input/InputAction.h"
#include "engine/input/Input.h"
#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"
#include "engine/render/debug/LightDebug3D.h"

namespace
{
const float kMoveSpeed = 4.0f;
const float kMouseLookSensitivity = 0.0025f;
const Vector3 kRingWorldOffset(-5.0f, 0.0f, 0.0f);
const Vector3 kSphereWorldOffset(5.0f, 0.0f, 0.0f);
const Vector3 kPointLightPosition = kSphereWorldOffset + Vector3(2.8f, 0.5f, 0.0f);
const Vector3 kSpotLightPosition = kSphereWorldOffset + Vector3(-5.2f, 2.4f, 2.0f);
const Vector3 kSpotLightDirection = (kSphereWorldOffset - kSpotLightPosition).normalized();
const float kSpotLightIntensity = 1.1f;
const float kSpotLightRange = 10.0f;
const float kSpotLightInnerConeCos = 0.96f;
const float kSpotLightOuterConeCos = 0.86f;
const uint32_t kSpotLightColor = 0xFFFFFFFF;

RingObjectDesc makeRingObjectDesc()
{
    RingObjectDesc desc;
    desc.worldOffset = kRingWorldOffset;
    desc.center = Vector2(400.0f, 300.0f);
    desc.ballStartPosition = Vector2(230.0f, 180.0f);
    desc.ballStartVelocity = Vector2(110.0f, -40.0f);
    desc.simulationScale = 100.0f;
    desc.borderRadiusPixels = 200.0f;
    desc.borderThicknessWorld = 0.02f;
    desc.ballRadiusPixels = 10.0f;
    desc.ballOutlineThicknessWorld = 0.02f;
    desc.planeThicknessWorld = 0.04f;
    desc.planeZ = 0.0f;
    desc.borderSegments = 96;
    desc.ballSegments = 48;
    desc.borderColor = 0xFFFFFFFF;
    desc.ballColor = 0xFFFFFFFF;
    desc.physicsMaterial = Material2D{0.0f, 0.9f, 0.08f, true};
    desc.borderMaterial.solid.color = desc.borderColor;
    desc.borderMaterial.wireframe.color = desc.borderColor;
    desc.borderMaterial.renderSolid = true;
    desc.borderMaterial.renderWireframe = false;
    desc.ballMaterial.solid.color = desc.ballColor;
    desc.ballMaterial.wireframe.color = desc.ballColor;
    desc.ballMaterial.renderSolid = true;
    desc.ballMaterial.renderWireframe = false;
    return desc;
}

SphereObjectDesc makeSphereObjectDesc()
{
    SphereObjectDesc desc;
    desc.worldOffset = kSphereWorldOffset;
    desc.boundaryRadius = 4.0f;
    desc.ballRadius = 0.45f;
    desc.ballStartPosition = Vector3(0.9f, 1.4f, -0.6f);
    desc.ballStartVelocity = Vector3(1.8f, -0.4f, 1.2f);
    desc.physicsMaterial = PhysicsMaterial3D{0.03f, 0.75f, 0.5f, true};
    desc.boundarySphereRings = 10;
    desc.boundarySphereSegments = 16;
    desc.ballSphereRings = 16;
    desc.ballSphereSegments = 24;
    desc.borderMaterial.solid.color = 0xFF9AD1FF;
    desc.borderMaterial.solid.opacity = 0.18f;
    desc.borderMaterial.solid.specularStrength = 0.05f;
    desc.borderMaterial.solid.shininess = 14.0f;
    desc.borderMaterial.wireframe.color = 0xFFBFE6FF;
    desc.borderMaterial.wireframe.opacity = 0.55f;
    desc.borderMaterial.renderSolid = true;
    desc.borderMaterial.renderWireframe = true;
    desc.ballMaterial.solid.color = 0xFFFFFFFF;
    desc.ballMaterial.solid.opacity = 1.0f;
    desc.ballMaterial.solid.emissiveColor = 0x00000000;
    desc.ballMaterial.solid.specularStrength = 1.0f;
    desc.ballMaterial.solid.shininess = 96.0f;
    desc.ballMaterial.wireframe.color = 0xFF707070;
    desc.ballMaterial.wireframe.opacity = 1.0f;
    desc.ballMaterial.renderSolid = true;
    desc.ballMaterial.renderWireframe = true;
    return desc;
}

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
    RingObjectDesc ringDesc = makeRingObjectDesc();
    ringObject = RingObject::create(ringDesc);

    SphereObjectDesc sphereDesc = makeSphereObjectDesc();
    sphereObject = SphereObject::create(sphereDesc);

    physicsWorld2D = std::make_unique<PhysicsWorld2D>();
    physicsWorld2D->setGravity(Vector2(0.0f, 980.0f));

    physicsWorld3D = std::make_unique<PhysicsWorld3D>();
    physicsWorld3D->setGravity(Vector3(0.0f, -7.5f, 0.0f));

    camera3D = std::make_unique<Camera3D>();
    camera3D->transform.position = Vector3(0.0f, 3.5f, 22.0f);
    camera3D->transform.rotation = Vector3(-0.12f, 0.0f, 0.0f);
    const Vector3 initialCameraForward = camera3D->getForward();

    combinedScene = std::make_unique<Scene3D>();
    combinedScene->createDirectionalLight({
        initialCameraForward.lengthSquared() > 0.0f ? initialCameraForward.normalized() : Vector3(0.0f, 0.0f, -1.0f),
        0xFFFFFFFF,
        0.55f});
    combinedScene->createPointLight({
        kPointLightPosition,
        0xFFFF4040,
        0.75f,
        8.0f});
    combinedScene->createSpotLight({
        kSpotLightPosition,
        kSpotLightDirection,
        kSpotLightColor,
        kSpotLightIntensity,
        kSpotLightRange,
        kSpotLightInnerConeCos,
        kSpotLightOuterConeCos});
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

    combinedScene->copyAmbientLightFromFirstAvailable({
        sphereObject ? &sphereObject->getRenderScene() : nullptr,
        ringObject ? &ringObject->getRenderScene() : nullptr});
    combinedScene->replaceEntitiesFrom({
        ringObject ? &ringObject->getRenderScene() : nullptr,
        sphereObject ? &sphereObject->getRenderScene() : nullptr});

    if (!showWireframes)
    {
        combinedScene->applyWireframeVisibilityOverride(false);
    }

    if (showLightDebugMarkers)
    {
        LightDebug3D::appendLightMarkers(*combinedScene, *combinedScene);
    }
}
