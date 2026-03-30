#include "app/demos/Demo.h"

#include <memory>

#include "app/scenes/PlaygroundScene.h"
#include "engine/input/Input.h"
#include "engine/input/InputAction.h"
#include "engine/math/Vector3.h"
#include "engine/render/debug/LightDebug3D.h"

namespace
{
const float kMoveSpeed = 4.0f;
const float kMouseLookSensitivity = 0.0025f;

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

void Demo::appendSceneAndRecordRange(const Scene3D &sourceScene, std::vector<SceneEntityRange> &ranges)
{
    if (!combinedScene)
    {
        return;
    }

    SceneEntityRange range;
    range.start = combinedScene->getEntities().size();
    range.count = sourceScene.getEntities().size();
    combinedScene->appendEntitiesFrom(sourceScene);
    ranges.push_back(range);
}

void Demo::copySceneRange(const Scene3D &sourceScene, const SceneEntityRange &range)
{
    if (!combinedScene || range.count == 0)
    {
        return;
    }

    std::vector<Entity3D> &targetEntities = combinedScene->getEntities();
    const std::vector<Entity3D> &sourceEntities = sourceScene.getEntities();
    if (sourceEntities.size() != range.count || range.start + range.count > targetEntities.size())
    {
        return;
    }

    for (std::size_t i = 0; i < range.count; ++i)
    {
        Entity3D &target = targetEntities[range.start + i];
        const Entity3D &source = sourceEntities[i];
        target.transform = source.transform;
        target.material = source.material;
        target.material.renderWireframe = showWireframes;
    }
}

void Demo::syncCombinedSceneEntities()
{
    if (!combinedScene)
    {
        return;
    }

    for (std::size_t i = 0; i < sphereObjects.size() && i < sphereObjectRanges.size(); ++i)
    {
        if (sphereObjects[i])
        {
            copySceneRange(sphereObjects[i]->getRenderScene(), sphereObjectRanges[i]);
        }
    }

    for (std::size_t i = 0; i < composedObjects.size() && i < composedObjectRanges.size(); ++i)
    {
        if (composedObjects[i])
        {
            copySceneRange(composedObjects[i]->getRenderScene(), composedObjectRanges[i]);
        }
    }

    combinedScene->touch();
}

void Demo::onAttach(int viewportWidth, int viewportHeight)
{
    this->viewportWidth = viewportWidth;
    this->viewportHeight = viewportHeight;

    const PlaygroundSceneDesc sceneDesc = makeDefaultPlaygroundSceneDesc();

    sphereObjects.clear();
    for (const SphereObjectDesc &sphereDesc : sceneDesc.sphereObjects)
    {
        sphereObjects.push_back(SphereObject::create(sphereDesc));
    }

    composedObjects.clear();
    for (const ComposedObject3DDesc &objectDesc : sceneDesc.composedObjects)
    {
        composedObjects.push_back(ComposedObject3D::create(objectDesc));
    }

    camera3D = std::make_unique<Camera3D>();
    camera3D->transform.position = sceneDesc.cameraPosition;
    camera3D->transform.rotation = sceneDesc.cameraRotation;

    combinedScene = std::make_unique<Scene3D>();
    combinedScene->getAmbientLight() = sceneDesc.ambientLight;
    for (const DirectionalLightDesc &lightDesc : sceneDesc.directionalLights)
    {
        combinedScene->createDirectionalLight(lightDesc);
    }
    for (const PointLightDesc &lightDesc : sceneDesc.pointLights)
    {
        combinedScene->createPointLight(lightDesc);
    }
    for (const SpotLightDesc &lightDesc : sceneDesc.spotLights)
    {
        combinedScene->createSpotLight(lightDesc);
    }

    rebuildCombinedScene();
    configureCamera(viewportWidth, viewportHeight);
}

void Demo::onResize(int viewportWidth, int viewportHeight)
{
    this->viewportWidth = viewportWidth;
    this->viewportHeight = viewportHeight;
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
    if (Input::wasKeyPressed(EngineKeyCode::R))
    {
        resetScene();
        return;
    }

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

void Demo::resetScene()
{
    Vector3 preservedCameraPosition = Vector3::zero();
    Vector3 preservedCameraRotation = Vector3::zero();
    const bool hadCamera = static_cast<bool>(camera3D);
    if (hadCamera)
    {
        preservedCameraPosition = camera3D->transform.position;
        preservedCameraRotation = camera3D->transform.rotation;
    }

    onAttach(viewportWidth, viewportHeight);

    if (hadCamera && camera3D)
    {
        camera3D->transform.position = preservedCameraPosition;
        camera3D->transform.rotation = preservedCameraRotation;
    }
}

void Demo::onFixedUpdate(float)
{
}

void Demo::onUpdate(float dt)
{
    updateDebugToggles();
    updateCamera(dt);
    syncCombinedSceneEntities();
}

void Demo::onRender(IGraphicsDevice &graphicsDevice) const
{
    if (!camera3D || !combinedScene)
    {
        return;
    }

    graphicsDevice.renderScene3D(*camera3D, *combinedScene);
}

std::string Demo::getRuntimeStatusText() const
{
    return {};
}

void Demo::rebuildCombinedScene()
{
    if (!combinedScene)
    {
        return;
    }

    combinedScene->clearEntities();
    sphereObjectRanges.clear();
    composedObjectRanges.clear();

    for (const auto &sphereObject : sphereObjects)
    {
        if (sphereObject)
        {
            appendSceneAndRecordRange(sphereObject->getRenderScene(), sphereObjectRanges);
        }
        else
        {
            sphereObjectRanges.push_back({});
        }
    }

    for (const auto &composedObject : composedObjects)
    {
        if (composedObject)
        {
            appendSceneAndRecordRange(composedObject->getRenderScene(), composedObjectRanges);
        }
        else
        {
            composedObjectRanges.push_back({});
        }
    }

    combinedScene->applyWireframeVisibilityOverride(showWireframes);

    if (showLightDebugMarkers)
    {
        LightDebug3D::appendLightMarkers(*combinedScene, *combinedScene);
    }
}
