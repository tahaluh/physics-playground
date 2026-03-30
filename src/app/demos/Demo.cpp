#include "app/demos/Demo.h"

#include <memory>

#include "app/scenes/PlaygroundScene.h"
#include "engine/input/Input.h"
#include "engine/input/InputAction.h"
#include "engine/math/Vector3.h"
namespace
{
const float kMoveSpeed = 4.0f;
const float kMouseLookSensitivity = 0.0025f;
const uint32_t kDefaultCubeColor = 0xFF555555;
const uint32_t kCollisionCubeColor = 0xFFD64545;

Quaternion makeCameraRotation(float pitch, float yaw)
{
    return Quaternion::fromEulerXYZ(Vector3(pitch, yaw, 0.0f));
}

Vector3 getPlanarForward(const Camera &camera)
{
    Vector3 forward = camera.getForward();
    forward.y = 0.0f;
    if (forward.lengthSquared() == 0.0f)
    {
        return Vector3(0.0f, 0.0f, -1.0f);
    }
    return forward.normalized();
}

Vector3 getPlanarRight(const Camera &camera)
{
    return Vector3::up().cross(getPlanarForward(camera) * -1.0f).normalized();
}

Material makeCubeMaterial(uint32_t color)
{
    Material material;
    material.solid = MaterialPresets::makePlastic(color, 0.42f);
    material.wireframe.baseColor = 0xFFFFFFFF;
    material.wireframe.opacity = 1.0f;
    material.wireframe.unlit = true;
    material.wireframe.ambientFactor = 0.0f;
    material.wireframe.diffuseFactor = 0.0f;
    material.renderSolid = true;
    material.renderWireframe = false;
    return material;
}
}

Demo::~Demo() = default;

void Demo::onAttach(int viewportWidth, int viewportHeight)
{
    this->viewportWidth = viewportWidth;
    this->viewportHeight = viewportHeight;

    const PlaygroundSceneDesc sceneDesc = makeDefaultPlaygroundSceneDesc();
    collisionCounts.clear();

    runtimeScene = std::make_unique<RuntimeScene>();
    for (const BodyObjectDesc &bodyDesc : sceneDesc.bodies)
    {
        BodyObject &body = runtimeScene->addBody(bodyDesc);
        body.onCollisionEnter = [this](BodyObject &self, BodyObject &, const CollisionPoints &) {
            const int nextCount = ++collisionCounts[&self];
            if (nextCount > 0)
            {
                self.setMaterial(makeCubeMaterial(kCollisionCubeColor));
            }
        };
        body.onCollision = [this](BodyObject &self, BodyObject &, const CollisionPoints &) {
            if (collisionCounts[&self] > 0)
            {
                self.setMaterial(makeCubeMaterial(kCollisionCubeColor));
            }
        };
        body.onCollisionExit = [this](BodyObject &self, BodyObject &, const CollisionPoints &) {
            auto it = collisionCounts.find(&self);
            if (it == collisionCounts.end())
            {
                self.setMaterial(makeCubeMaterial(kDefaultCubeColor));
                return;
            }

            it->second = std::max(0, it->second - 1);
            if (it->second == 0)
            {
                self.setMaterial(makeCubeMaterial(kDefaultCubeColor));
            }
        };
    }

    camera3D = std::make_unique<Camera>();
    camera3D->transform.position = sceneDesc.cameraPosition;
    camera3D->transform.rotation = sceneDesc.cameraRotation;
    cameraPitch = 0.0f;
    cameraYaw = 0.0f;
    initialCameraRotation = sceneDesc.cameraRotation;

    runtimeScene->getRenderScene().getAmbientLight() = sceneDesc.ambientLight;
    for (const DirectionalLightDesc &lightDesc : sceneDesc.directionalLights)
    {
        runtimeScene->getRenderScene().createDirectionalLight(lightDesc);
    }
    for (const PointLightDesc &lightDesc : sceneDesc.pointLights)
    {
        runtimeScene->getRenderScene().createPointLight(lightDesc);
    }
    for (const SpotLightDesc &lightDesc : sceneDesc.spotLights)
    {
        runtimeScene->getRenderScene().createSpotLight(lightDesc);
    }
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

    cameraYaw -= Input::getMouseDeltaX() * kMouseLookSensitivity;
    cameraPitch -= Input::getMouseDeltaY() * kMouseLookSensitivity;
    cameraPitch = Vector3::clamp(cameraPitch, -1.4f, 1.4f);
    camera3D->transform.rotation = (makeCameraRotation(cameraPitch, cameraYaw) * initialCameraRotation).normalized();
}

void Demo::updateDebugToggles()
{
    if (Input::wasKeyPressed(EngineKeyCode::R))
    {
        resetScene();
        return;
    }

    if (Input::wasKeyPressed(EngineKeyCode::Space))
    {
        simulationPaused = !simulationPaused;
    }

    if (Input::wasActionPressed(EngineInputAction::ToggleLightDebug))
    {
        showLightDebugMarkers = !showLightDebugMarkers;
    }

    if (Input::wasActionPressed(EngineInputAction::ToggleWireframe))
    {
        showWireframes = !showWireframes;
    }
}

void Demo::resetScene()
{
    Vector3 preservedCameraPosition = Vector3::zero();
    Quaternion preservedCameraRotation = Quaternion::identity();
    const float preservedCameraPitch = cameraPitch;
    const float preservedCameraYaw = cameraYaw;
    const Quaternion preservedInitialCameraRotation = initialCameraRotation;
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
        cameraPitch = preservedCameraPitch;
        cameraYaw = preservedCameraYaw;
        initialCameraRotation = preservedInitialCameraRotation;
    }
}

void Demo::onFixedUpdate(float dt)
{
    if (runtimeScene && !simulationPaused)
    {
        runtimeScene->step(dt);
    }
}

void Demo::onUpdate(float dt)
{
    updateDebugToggles();
    updateCamera(dt);
}

void Demo::onRender(IGraphicsDevice &graphicsDevice) const
{
    if (!camera3D || !runtimeScene)
    {
        return;
    }

    graphicsDevice.setLightDebugOverlayEnabled(showLightDebugMarkers);
    graphicsDevice.setWireframeOverlayEnabled(showWireframes);
    graphicsDevice.renderScene(*camera3D, runtimeScene->getRenderScene());
}

std::string Demo::getRuntimeStatusText() const
{
    return {};
}
