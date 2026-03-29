#include "app/demos/Demo.h"

#include <memory>

#include "app/scenes/PlaygroundScene.h"
#include "engine/input/InputAction.h"
#include "engine/input/Input.h"
#include "engine/math/Vector3.h"
#include "engine/render/debug/LightDebug3D.h"
#include "engine/render/debug/PhysicsDebug3D.h"

namespace
{
    const float kMoveSpeed = 4.0f;
    const float kMouseLookSensitivity = 0.0025f;
    const float kPhysicsDebugSolidOpacity = 0.24f;
    const float kPhysicsDebugWireframeOpacity = 0.12f;

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

    void applyPhysicsDebugTransparency(Scene3D &scene)
    {
        for (Entity3D &entity : scene.getEntities())
        {
            if (entity.material.renderSolid)
            {
                entity.material.solid.opacity = std::min(entity.material.solid.opacity, kPhysicsDebugSolidOpacity);
            }

            if (entity.material.renderWireframe)
            {
                entity.material.wireframe.opacity = std::min(entity.material.wireframe.opacity, kPhysicsDebugWireframeOpacity);
            }
        }
    }
}

Demo::~Demo() = default;

void Demo::onAttach(int viewportWidth, int viewportHeight)
{
    const PlaygroundSceneDesc sceneDesc = makeDefaultPlaygroundSceneDesc();

    ringObjects.clear();
    for (const RingObjectDesc &ringDesc : sceneDesc.ringObjects)
    {
        ringObjects.push_back(RingObject::create(ringDesc));
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

    physicsWorld3D = std::make_unique<PhysicsWorld3D>();
    physicsWorld3D->setGravity(sceneDesc.gravity3D);
    physicsWorld3D->setStopThreshold(0.02f);
    physicsWorld3D->setAngularStopThreshold(0.035f);
    physicsWorld3D->setRestitutionThreshold(0.22f);
    physicsWorld3D->setSleepDelay(0.45f);
    physicsWorld3D->setSolverIterations(8);

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
    if (physicsWorld2D)
    {
        for (const auto &ringObject : ringObjects)
        {
            if (ringObject)
            {
                ringObject->step(*physicsWorld2D, dt);
            }
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
    for (const auto &ringObject : ringObjects)
    {
        if (ringObject)
        {
            combinedScene->appendEntitiesFrom(ringObject->getRenderScene());
        }
    }
    for (const auto &sphereObject : sphereObjects)
    {
        if (sphereObject)
        {
            combinedScene->appendEntitiesFrom(sphereObject->getRenderScene());
        }
    }
    for (const auto &sphereArenaObject : sphereArenaObjects)
    {
        if (sphereArenaObject)
        {
            combinedScene->appendEntitiesFrom(sphereArenaObject->getRenderScene());
        }
    }
    for (const auto &composedObject : composedObjects)
    {
        if (composedObject)
        {
            combinedScene->appendEntitiesFrom(composedObject->getRenderScene());
        }
    }

    combinedScene->applyWireframeVisibilityOverride(showWireframes);

    if (showPhysicsDebugMarkers)
    {
        applyPhysicsDebugTransparency(*combinedScene);
    }

    if (showLightDebugMarkers)
    {
        LightDebug3D::appendLightMarkers(*combinedScene, *combinedScene);
    }

    if (showPhysicsDebugMarkers)
    {
        for (const auto &ringObject : ringObjects)
        {
            if (ringObject)
            {
                ringObject->appendDebugMarkers(*combinedScene);
            }
        }

        for (const auto &sphereArenaObject : sphereArenaObjects)
        {
            if (sphereArenaObject)
            {
                PhysicsDebug3D::appendPhysicsSceneMarkers(
                    sphereArenaObject->getPhysicsScene(),
                    *combinedScene,
                    sphereArenaObject->getWorldOffset());
            }
        }

        for (const auto &composedObject : composedObjects)
        {
            if (composedObject)
            {
                PhysicsDebug3D::appendPhysicsSceneMarkers(
                    composedObject->getPhysicsScene(),
                    *combinedScene,
                    composedObject->getWorldOffset());
            }
        }
    }
}
