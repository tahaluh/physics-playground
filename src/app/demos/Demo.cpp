#include "app/demos/Demo.h"

#include <memory>

#include "app/scenes/PlaygroundScene.h"
#include "engine/input/InputAction.h"
#include "engine/input/Input.h"
#include "engine/math/Vector3.h"
#include "engine/render/debug/DebugSceneComposer3D.h"

namespace
{
    const float kMoveSpeed = 4.0f;
    const float kMouseLookSensitivity = 0.0025f;
    const float kDebugRebuildInterval = 1.0f / 12.0f;

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

    for (std::size_t i = 0; i < ringObjects.size() && i < ringObjectRanges.size(); ++i)
    {
        if (ringObjects[i])
        {
            copySceneRange(ringObjects[i]->getRenderScene(), ringObjectRanges[i]);
        }
    }

    for (std::size_t i = 0; i < squareObjects.size() && i < squareObjectRanges.size(); ++i)
    {
        if (squareObjects[i])
        {
            copySceneRange(squareObjects[i]->getRenderScene(), squareObjectRanges[i]);
        }
    }

    for (std::size_t i = 0; i < sphereObjects.size() && i < sphereObjectRanges.size(); ++i)
    {
        if (sphereObjects[i])
        {
            copySceneRange(sphereObjects[i]->getRenderScene(), sphereObjectRanges[i]);
        }
    }

    for (std::size_t i = 0; i < sphereArenaObjects.size() && i < sphereArenaObjectRanges.size(); ++i)
    {
        if (sphereArenaObjects[i])
        {
            copySceneRange(sphereArenaObjects[i]->getRenderScene(), sphereArenaObjectRanges[i]);
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

Demo::~Demo() = default;

void Demo::onAttach(int viewportWidth, int viewportHeight)
{
    const PlaygroundSceneDesc sceneDesc = makeDefaultPlaygroundSceneDesc();
    const std::size_t simulationCount = sceneDesc.ringObjects.size() + sceneDesc.sphereArenaObjects.size();
    const bool isLargeSimulationBenchmark = simulationCount >= 64;

    ringObjects.clear();
    for (const RingObjectDesc &ringDesc : sceneDesc.ringObjects)
    {
        ringObjects.push_back(RingObject::create(ringDesc));
    }

    squareObjects.clear();
    squareObjectRingIndices.clear();
    for (const SquareObjectDesc &squareDesc : sceneDesc.squareObjects)
    {
        if (squareDesc.ringObjectIndex >= ringObjects.size() || !ringObjects[squareDesc.ringObjectIndex])
        {
            continue;
        }

        squareObjects.push_back(SquareObject::create(
            squareDesc,
            ringObjects[squareDesc.ringObjectIndex]->getPhysicsScene(),
            ringObjects[squareDesc.ringObjectIndex]->getConfig()));
        squareObjectRingIndices.push_back(squareDesc.ringObjectIndex);
    }

    sphereArenaObjects.clear();
    for (const SphereArenaObjectDesc &arenaDesc : sceneDesc.sphereArenaObjects)
    {
        sphereArenaObjects.push_back(SphereArenaObject::create(arenaDesc));
    }

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

    physicsWorld2D = std::make_unique<PhysicsWorld2D>();
    physicsWorld2D->setGravity(sceneDesc.gravity2D);
    physicsWorld2D->setSolverIterations(isLargeSimulationBenchmark ? 4 : 8);
    physicsWorld2D->setStopThreshold(isLargeSimulationBenchmark ? 12.0f : 8.0f);
    physicsWorld2D->setAngularStopThreshold(isLargeSimulationBenchmark ? 0.18f : 0.12f);
    physicsWorld2D->setSleepDelay(isLargeSimulationBenchmark ? 0.22f : 0.35f);

    physicsWorld3D = std::make_unique<PhysicsWorld3D>();
    physicsWorld3D->setGravity(sceneDesc.gravity3D);
    physicsWorld3D->setStopThreshold(isLargeSimulationBenchmark ? 0.035f : 0.02f);
    physicsWorld3D->setAngularStopThreshold(isLargeSimulationBenchmark ? 0.055f : 0.035f);
    physicsWorld3D->setRestitutionThreshold(0.22f);
    physicsWorld3D->setSleepDelay(isLargeSimulationBenchmark ? 0.28f : 0.45f);
    physicsWorld3D->setSolverIterations(isLargeSimulationBenchmark ? 4 : 8);

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

    if (Input::wasKeyPressed(EngineKeyCode::Space))
    {
        freezeFixedTicks = !freezeFixedTicks;
        debugRebuildAccumulator = 0.0f;
        if (showLightDebugMarkers || showPhysicsDebugMarkers)
        {
            changed = true;
        }
    }

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

    if (Input::wasActionPressed(EngineInputAction::TogglePhysicsDebug))
    {
        showPhysicsDebugMarkers = !showPhysicsDebugMarkers;
        changed = true;
    }

    if (changed)
    {
        rebuildCombinedScene();
    }
}

void Demo::onFixedUpdate(float dt)
{
    if (freezeFixedTicks)
    {
        return;
    }

    if (physicsWorld2D)
    {
        for (const auto &ringObject : ringObjects)
        {
            if (ringObject)
            {
                ringObject->step(*physicsWorld2D, dt);
            }
        }

        for (std::size_t i = 0; i < squareObjects.size(); ++i)
        {
            if (!squareObjects[i] || i >= squareObjectRingIndices.size())
            {
                continue;
            }

            const std::size_t ringIndex = squareObjectRingIndices[i];
            if (ringIndex >= ringObjects.size() || !ringObjects[ringIndex])
            {
                continue;
            }

            squareObjects[i]->syncRenderScene(
                ringObjects[ringIndex]->getPhysicsScene(),
                ringObjects[ringIndex]->getConfig());
        }
    }

    if (physicsWorld3D)
    {
        for (const auto &sphereArenaObject : sphereArenaObjects)
        {
            if (sphereArenaObject)
            {
                sphereArenaObject->step(*physicsWorld3D, dt);
            }
        }

        for (const auto &sphereObject : sphereObjects)
        {
            if (sphereObject)
            {
                sphereObject->syncRenderScene();
            }
        }

        for (const auto &composedObject : composedObjects)
        {
            if (composedObject)
            {
                composedObject->step(*physicsWorld3D, dt);
            }
        }
    }

    if (showLightDebugMarkers || showPhysicsDebugMarkers)
    {
        const bool needsRealtimePhysicsDebug = showPhysicsDebugMarkers && !freezeFixedTicks;
        debugRebuildAccumulator += dt;
        if (needsRealtimePhysicsDebug || debugRebuildAccumulator >= kDebugRebuildInterval)
        {
            rebuildCombinedScene();
            debugRebuildAccumulator = 0.0f;
        }
        else
        {
            syncCombinedSceneEntities();
        }
    }
    else
    {
        debugRebuildAccumulator = 0.0f;
        syncCombinedSceneEntities();
    }
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
    ringObjectRanges.clear();
    squareObjectRanges.clear();
    sphereObjectRanges.clear();
    sphereArenaObjectRanges.clear();
    composedObjectRanges.clear();

    for (const auto &ringObject : ringObjects)
    {
        if (ringObject)
        {
            appendSceneAndRecordRange(ringObject->getRenderScene(), ringObjectRanges);
        }
        else
        {
            ringObjectRanges.push_back({});
        }
    }
    for (const auto &squareObject : squareObjects)
    {
        if (squareObject)
        {
            appendSceneAndRecordRange(squareObject->getRenderScene(), squareObjectRanges);
        }
        else
        {
            squareObjectRanges.push_back({});
        }
    }
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
    for (const auto &sphereArenaObject : sphereArenaObjects)
    {
        if (sphereArenaObject)
        {
            appendSceneAndRecordRange(sphereArenaObject->getRenderScene(), sphereArenaObjectRanges);
        }
        else
        {
            sphereArenaObjectRanges.push_back({});
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

    DebugSceneComposer3D::appendOverlays(
        *combinedScene,
        DebugSceneComposer3D::Options{
            showLightDebugMarkers,
            showPhysicsDebugMarkers,
            0.24f,
            0.12f},
        ringObjects,
        squareObjects,
        squareObjectRingIndices,
        sphereArenaObjects,
        composedObjects);
}
