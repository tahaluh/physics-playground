#include "app/demos/RenderDemo3D.h"

#include <memory>

#include "engine/graphics/IGraphicsDevice.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Vector3.h"
#include "engine/physics/2d/BallBody2D.h"
#include "engine/physics/2d/BorderCircleBody2D.h"
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
    const Vector2 kSimulationCenter(400.0f, 300.0f);
    constexpr float kSimulationScale = 100.0f;
    constexpr float kBorderRadiusPixels = 200.0f;
    constexpr float kBorderThicknessWorld = 0.02f;
    constexpr float kBallRadiusPixels = 10.0f;
    constexpr float kBallOutlineThicknessWorld = 0.02f;
    constexpr float kMoveSpeed = 3.5f;
    constexpr float kMouseLookSensitivity = 0.0025f;
    constexpr float kMinimumCameraDistance = 2.5f;
    constexpr float kBallControlForce = 850.0f;
    constexpr float kSimulationPlaneZ = 0.0f;

    Vector3 toWorldPlanePosition(const Vector2 &simulationPosition, float zOffset = 0.0f)
    {
        const float worldX = (simulationPosition.x - kSimulationCenter.x) / kSimulationScale;
        const float worldY = -(simulationPosition.y - kSimulationCenter.y) / kSimulationScale;
        return Vector3(worldX, worldY, zOffset);
    }

    float toWorldRadius(float simulationRadius)
    {
        return simulationRadius / kSimulationScale;
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
    physicsScene = std::make_unique<Scene2D>();
    physicsWorld = std::make_unique<PhysicsWorld2D>();
    scene = std::make_unique<Scene3D>();

    physicsWorld->setGravity(Vector2(0.0f, 980.0f));

    physicsScene->addBody(std::make_unique<BorderCircleBody2D>(kSimulationCenter, kBorderRadiusPixels));
    physicsScene->addBody(std::make_unique<BallBody2D>(kBallRadiusPixels, Vector2(230.0f, 180.0f), Vector2(110.0f, -40.0f), 0xFFFFFFFF));

    for (const auto &body : physicsScene->getBodies())
    {
        if (!body->isStatic())
        {
            controlledBall = body.get();
            break;
        }
    }

    Entity3D border;
    border.name = "PhysicsBorder";
    border.mesh = MeshFactory3D::makeRing(
        toWorldRadius(kBorderRadiusPixels) - kBorderThicknessWorld,
        toWorldRadius(kBorderRadiusPixels),
        96,
        0xFFFFFFFF);
    border.material.fillColor = 0xFFFFFFFF;
    border.material.wireframeColor = 0xFFFFFFFF;
    border.material.renderSolid = true;
    border.material.renderWireframe = false;
    border.transform.position = toWorldPlanePosition(kSimulationCenter, kSimulationPlaneZ);
    borderEntity = &scene->createEntity(border);

    for (const auto &body : physicsScene->getBodies())
    {
        if (body->isStatic())
        {
            continue;
        }

        const auto *circle = dynamic_cast<const CircleShape *>(body->getShape());
        if (!circle)
        {
            continue;
        }

        Entity3D ball;
        ball.name = "PhysicsBall";
        const float ballOuterRadius = circle->getRadius() / kSimulationScale;
        const float ballInnerRadius = ballOuterRadius > kBallOutlineThicknessWorld
                                          ? ballOuterRadius - kBallOutlineThicknessWorld
                                          : ballOuterRadius * 0.5f;
        ball.mesh = MeshFactory3D::makeRing(ballInnerRadius, ballOuterRadius, 48, body->getColor());
        ball.material.fillColor = body->getColor();
        ball.material.wireframeColor = body->getColor();
        ball.material.renderSolid = true;
        ball.material.renderWireframe = false;
        ball.transform.position = toWorldPlanePosition(body->getPosition(), kSimulationPlaneZ);
        ballEntities.push_back(&scene->createEntity(ball));
    }

    syncSceneFromPhysics();
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

void RenderDemo3D::syncSceneFromPhysics()
{
    if (!physicsScene)
    {
        return;
    }

    size_t ballIndex = 0;
    for (const auto &body : physicsScene->getBodies())
    {
        if (body->isStatic())
        {
            if (borderEntity)
            {
                borderEntity->transform.position = toWorldPlanePosition(body->getPosition(), kSimulationPlaneZ);
                borderEntity->transform.rotation = Vector3::zero();
                borderEntity->transform.scale = Vector3::one();
            }
            continue;
        }

        if (ballIndex >= ballEntities.size())
        {
            continue;
        }

        Entity3D *entity = ballEntities[ballIndex++];
        entity->transform.position = toWorldPlanePosition(body->getPosition(), kSimulationPlaneZ);
        entity->transform.rotation = Vector3::zero();
        entity->transform.scale = Vector3::one();
    }
}

void RenderDemo3D::onFixedUpdate(float dt)
{
    updateCamera(dt);
    applyBallControl();

    if (physicsWorld && physicsScene)
    {
        physicsWorld->step(*physicsScene, dt);
    }

    syncSceneFromPhysics();
}

void RenderDemo3D::onRender(IGraphicsDevice &graphicsDevice) const
{
    if (!scene || !camera)
    {
        return;
    }

    graphicsDevice.renderScene3D(*camera, *scene);
}
