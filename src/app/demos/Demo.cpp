#include "app/demos/Demo.h"

#include <algorithm>
#include <memory>

#include "engine/graphics/IGraphicsDevice.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Vector2.h"
#include "engine/physics/2d/BallBody2D.h"
#include "engine/physics/2d/BorderCircleBody2D.h"
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/PhysicsWorld2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include "engine/physics/3d/BallBody3D.h"
#include "engine/physics/3d/BorderSphereBody3D.h"
#include "engine/physics/3d/PhysicsBody3D.h"
#include "engine/physics/3d/PhysicsWorld3D.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/2d/Scene2D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/PhysicsScene3D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
const float kMoveSpeed = 4.0f;
const float kMouseLookSensitivity = 0.0025f;
const Vector3 kRingWorldOffset(-5.0f, 0.0f, 0.0f);
const Vector3 kSphereWorldOffset(5.0f, 0.0f, 0.0f);

const Vector2 kRingSimulationCenter(400.0f, 300.0f);
constexpr float kRingSimulationScale = 100.0f;
constexpr float kRingBorderRadiusPixels = 200.0f;
constexpr float kRingBorderThicknessWorld = 0.02f;
constexpr float kRingBallRadiusPixels = 10.0f;
constexpr float kRingBallOutlineThicknessWorld = 0.02f;
constexpr float kRingPlaneThicknessWorld = 0.04f;
constexpr float kRingPlaneZ = 0.0f;

constexpr float kSphereBoundaryRadius = 4.0f;
constexpr float kSphereBallRadius = 0.45f;
const Vector3 kSphereBallStartPosition(0.9f, 1.4f, -0.6f);
const Vector3 kSphereBallStartVelocity(1.8f, -0.4f, 1.2f);
constexpr float kSphereBallLinearDamping = 0.03f;
constexpr float kSphereBallRestitution = 0.68f;
constexpr float kSphereBallSurfaceFriction = 0.48f;
constexpr int kBoundarySphereRings = 20;
constexpr int kBoundarySphereSegments = 32;
constexpr int kBallSphereRings = 16;
constexpr int kBallSphereSegments = 24;

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

Vector3 toRingWorldPosition(const Vector2 &simulationPosition, float zOffset = 0.0f)
{
    const float worldX = (simulationPosition.x - kRingSimulationCenter.x) / kRingSimulationScale;
    const float worldY = -(simulationPosition.y - kRingSimulationCenter.y) / kRingSimulationScale;
    return Vector3(worldX, worldY, zOffset);
}

float toRingWorldRadius(float simulationRadius)
{
    return simulationRadius / kRingSimulationScale;
}
}

Demo::~Demo() = default;

void Demo::onAttach(int viewportWidth, int viewportHeight)
{
    createRingObject();
    createSphereObject();

    physicsWorld2D = std::make_unique<PhysicsWorld2D>();
    physicsWorld2D->setGravity(Vector2(0.0f, 980.0f));
    physicsWorld3D = std::make_unique<PhysicsWorld3D>();
    physicsWorld3D->setGravity(Vector3(0.0f, -7.5f, 0.0f));

    camera3D = std::make_unique<Camera3D>();
    camera3D->transform.position = Vector3(0.0f, 3.5f, 22.0f);
    camera3D->transform.rotation = Vector3(-0.12f, 0.0f, 0.0f);

    combinedScene = std::make_unique<Scene3D>();
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

    if (physicsWorld2D && ringPhysicsScene)
    {
        physicsWorld2D->step(*ringPhysicsScene, dt);
        syncRingRenderScene();
    }

    if (physicsWorld3D && spherePhysicsScene)
    {
        physicsWorld3D->step(*spherePhysicsScene, dt);
        syncSphereRenderScene();
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

void Demo::createRingObject()
{
    ringPhysicsScene = std::make_unique<Scene2D>();
    ringRenderScene = std::make_unique<Scene3D>();
    ringRenderScene->getAmbientLight().color = 0xFFFFFFFF;
    ringRenderScene->getAmbientLight().intensity = 1.0f;
    ringWorldOffset = kRingWorldOffset;

    ringBorderBodyIndex = ringPhysicsScene->getBodies().size();
    ringPhysicsScene->addBody(std::make_unique<BorderCircleBody2D>(kRingSimulationCenter, kRingBorderRadiusPixels));

    ringControlledBodyIndex = ringPhysicsScene->getBodies().size();
    ringPhysicsScene->addBody(
        std::make_unique<BallBody2D>(
            kRingBallRadiusPixels,
            Vector2(230.0f, 180.0f),
            Vector2(110.0f, -40.0f),
            0xFFFFFFFF));

    Entity3D border;
    border.name = "PhysicsBorder";
    border.mesh = MeshFactory3D::makeDoubleSidedRing(
        toRingWorldRadius(kRingBorderRadiusPixels) - kRingBorderThicknessWorld,
        toRingWorldRadius(kRingBorderRadiusPixels),
        kRingPlaneThicknessWorld,
        96,
        0xFFFFFFFF);
    border.material.solid.color = 0xFFFFFFFF;
    border.material.wireframe.color = 0xFFFFFFFF;
    border.material.renderSolid = true;
    border.material.renderWireframe = false;
    border.transform.position = toRingWorldPosition(kRingSimulationCenter, kRingPlaneZ);
    ringBorderEntityIndex = ringRenderScene->getEntities().size();
    ringRenderScene->createEntity(border);

    const auto &bodies = ringPhysicsScene->getBodies();
    for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex)
    {
        const auto &body = bodies[bodyIndex];
        if (body->isStatic())
        {
            continue;
        }

        const auto *circle = dynamic_cast<const CircleShape *>(body->getShape());
        if (!circle)
        {
            continue;
        }

        ringDynamicBodyIndices.push_back(bodyIndex);

        Entity3D ball;
        ball.name = "PhysicsBall";
        const float ballOuterRadius = circle->getRadius() / kRingSimulationScale;
        const float ballInnerRadius = ballOuterRadius > kRingBallOutlineThicknessWorld
                                          ? ballOuterRadius - kRingBallOutlineThicknessWorld
                                          : ballOuterRadius * 0.5f;
        ball.mesh = MeshFactory3D::makeDoubleSidedRing(
            ballInnerRadius,
            ballOuterRadius,
            kRingPlaneThicknessWorld,
            48,
            body->getColor());
        ball.material.solid.color = body->getColor();
        ball.material.wireframe.color = body->getColor();
        ball.material.renderSolid = true;
        ball.material.renderWireframe = false;
        ball.transform.position = toRingWorldPosition(body->getPosition(), kRingPlaneZ);

        ringDynamicEntityIndices.push_back(ringRenderScene->getEntities().size());
        ringRenderScene->createEntity(ball);
    }

    syncRingRenderScene();
}

void Demo::createSphereObject()
{
    spherePhysicsScene = std::make_unique<PhysicsScene3D>();
    sphereRenderScene = std::make_unique<Scene3D>();
    sphereRenderScene->getAmbientLight().color = 0xFFFFFFFF;
    sphereRenderScene->getAmbientLight().intensity = 1.0f;
    sphereWorldOffset = kSphereWorldOffset;

    sphereBorderBodyIndex = spherePhysicsScene->getBodies().size();
    spherePhysicsScene->addBody(std::make_unique<BorderSphereBody3D>(Vector3::zero(), kSphereBoundaryRadius));

    sphereBallBodyIndex = spherePhysicsScene->getBodies().size();
    auto ballBody = std::make_unique<BallBody3D>(kSphereBallRadius, kSphereBallStartPosition, kSphereBallStartVelocity);
    ballBody->getMaterial().linearDamping = kSphereBallLinearDamping;
    ballBody->getMaterial().restitution = kSphereBallRestitution;
    ballBody->getMaterial().surfaceFriction = kSphereBallSurfaceFriction;
    spherePhysicsScene->addBody(std::move(ballBody));

    Entity3D border;
    border.name = "BoundarySphere";
    border.mesh = MeshFactory3D::makeSphere(kSphereBoundaryRadius, kBoundarySphereRings, kBoundarySphereSegments, 0);
    border.material.solid.color = 0xFF9AD1FF;
    border.material.solid.opacity = 0.18f;
    border.material.wireframe.color = 0xFFBFE6FF;
    border.material.wireframe.opacity = 0.55f;
    border.material.renderSolid = true;
    border.material.renderWireframe = true;
    sphereBorderEntityIndex = sphereRenderScene->getEntities().size();
    sphereRenderScene->createEntity(border);

    Entity3D ball;
    ball.name = "InnerBall";
    ball.mesh = MeshFactory3D::makeSphere(kSphereBallRadius, kBallSphereRings, kBallSphereSegments, 0);
    ball.material.solid.color = 0xFFFF9F1C;
    ball.material.solid.opacity = 1.0f;
    ball.material.solid.emissiveColor = 0x14080400;
    ball.material.wireframe.color = 0xFFFFD166;
    ball.material.wireframe.opacity = 1.0f;
    ball.material.renderSolid = true;
    ball.material.renderWireframe = true;
    sphereBallEntityIndex = sphereRenderScene->getEntities().size();
    sphereRenderScene->createEntity(ball);

    syncSphereRenderScene();
}

void Demo::syncRingRenderScene()
{
    if (!ringPhysicsScene || !ringRenderScene)
    {
        return;
    }

    auto &bodies = ringPhysicsScene->getBodies();
    auto &entities = ringRenderScene->getEntities();

    if (ringBorderBodyIndex < bodies.size() && ringBorderEntityIndex < entities.size())
    {
        entities[ringBorderEntityIndex].transform.position =
            toRingWorldPosition(bodies[ringBorderBodyIndex]->getPosition(), kRingPlaneZ) + ringWorldOffset;
        entities[ringBorderEntityIndex].transform.rotation =
            Vector3(0.0f, 0.0f, bodies[ringBorderBodyIndex]->getRotationAngle());
        entities[ringBorderEntityIndex].transform.scale = Vector3::one();
    }

    const std::size_t mappedCount = std::min(ringDynamicBodyIndices.size(), ringDynamicEntityIndices.size());
    for (std::size_t i = 0; i < mappedCount; ++i)
    {
        const std::size_t bodyIndex = ringDynamicBodyIndices[i];
        const std::size_t entityIndex = ringDynamicEntityIndices[i];
        if (bodyIndex >= bodies.size() || entityIndex >= entities.size())
        {
            continue;
        }

        entities[entityIndex].transform.position =
            toRingWorldPosition(bodies[bodyIndex]->getPosition(), kRingPlaneZ) + ringWorldOffset;
        entities[entityIndex].transform.rotation = Vector3(0.0f, 0.0f, bodies[bodyIndex]->getRotationAngle());
        entities[entityIndex].transform.scale = Vector3::one();
    }
}

void Demo::syncSphereRenderScene()
{
    if (!spherePhysicsScene || !sphereRenderScene)
    {
        return;
    }

    auto &bodies = spherePhysicsScene->getBodies();
    auto &entities = sphereRenderScene->getEntities();

    if (sphereBorderBodyIndex < bodies.size() && sphereBorderEntityIndex < entities.size())
    {
        entities[sphereBorderEntityIndex].transform.position = bodies[sphereBorderBodyIndex]->getPosition() + sphereWorldOffset;
        entities[sphereBorderEntityIndex].transform.rotation = bodies[sphereBorderBodyIndex]->getRotation();
        entities[sphereBorderEntityIndex].transform.clearCustomRotationMatrix();
        entities[sphereBorderEntityIndex].transform.scale = Vector3::one();
    }

    if (sphereBallBodyIndex < bodies.size() && sphereBallEntityIndex < entities.size())
    {
        entities[sphereBallEntityIndex].transform.position = bodies[sphereBallBodyIndex]->getPosition() + sphereWorldOffset;
        entities[sphereBallEntityIndex].transform.rotation = Vector3::zero();
        entities[sphereBallEntityIndex].transform.setCustomRotationMatrix(bodies[sphereBallBodyIndex]->getRotationMatrix());
        entities[sphereBallEntityIndex].transform.scale = Vector3::one();
    }
}

void Demo::rebuildCombinedScene()
{
    if (!combinedScene)
    {
        return;
    }

    auto &combinedEntities = combinedScene->getEntities();
    combinedEntities.clear();

    if (sphereRenderScene)
    {
        combinedScene->getAmbientLight() = sphereRenderScene->getAmbientLight();
    }
    else if (ringRenderScene)
    {
        combinedScene->getAmbientLight() = ringRenderScene->getAmbientLight();
    }

    const auto appendSceneEntities = [&](const Scene3D &sourceScene)
    {
        const auto &sourceEntities = sourceScene.getEntities();
        combinedEntities.insert(combinedEntities.end(), sourceEntities.begin(), sourceEntities.end());
    };

    if (ringRenderScene)
    {
        appendSceneEntities(*ringRenderScene);
    }

    if (sphereRenderScene)
    {
        appendSceneEntities(*sphereRenderScene);
    }
}
