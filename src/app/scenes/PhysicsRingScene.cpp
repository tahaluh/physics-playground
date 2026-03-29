#include "app/scenes/PhysicsRingScene.h"

#include <memory>

#include "engine/math/Vector3.h"
#include "engine/physics/2d/BallBody2D.h"
#include "engine/physics/2d/BorderCircleBody2D.h"
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/2d/Scene2D.h"
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
constexpr float kPlaneThicknessWorld = 0.04f;
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
}

PhysicsRingScene::~PhysicsRingScene() = default;

std::unique_ptr<PhysicsRingScene> PhysicsRingScene::createDefault(bool enableRenderScene)
{
    auto loadedScene = std::make_unique<PhysicsRingScene>();
    loadedScene->physicsScene = std::make_unique<Scene2D>();
    if (enableRenderScene)
    {
        loadedScene->renderScene = std::make_unique<Scene3D>();
    }

    loadedScene->borderBodyIndex = loadedScene->getPhysicsScene().getBodies().size();
    loadedScene->getPhysicsScene().addBody(std::make_unique<BorderCircleBody2D>(kSimulationCenter, kBorderRadiusPixels));

    loadedScene->controlledBodyIndex = loadedScene->getPhysicsScene().getBodies().size();
    loadedScene->getPhysicsScene().addBody(
        std::make_unique<BallBody2D>(
            kBallRadiusPixels,
            Vector2(230.0f, 180.0f),
            Vector2(110.0f, -40.0f),
            0xFFFFFFFF));

    if (loadedScene->renderScene)
    {
        Entity3D border;
        border.name = "PhysicsBorder";
        border.mesh = MeshFactory3D::makeDoubleSidedRing(
            toWorldRadius(kBorderRadiusPixels) - kBorderThicknessWorld,
            toWorldRadius(kBorderRadiusPixels),
            kPlaneThicknessWorld,
            96,
            0xFFFFFFFF);
        border.material.solid.color = 0xFFFFFFFF;
        border.material.wireframe.color = 0xFFFFFFFF;
        border.material.renderSolid = true;
        border.material.renderWireframe = false;
        border.transform.position = toWorldPlanePosition(kSimulationCenter, kSimulationPlaneZ);

        loadedScene->borderEntityIndex = loadedScene->getRenderScene().getEntities().size();
        loadedScene->getRenderScene().createEntity(border);
    }

    const auto &bodies = loadedScene->getPhysicsScene().getBodies();
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

        loadedScene->dynamicBodyIndices.push_back(bodyIndex);

        if (loadedScene->renderScene)
        {
            Entity3D ball;
            ball.name = "PhysicsBall";
            const float ballOuterRadius = circle->getRadius() / kSimulationScale;
            const float ballInnerRadius = ballOuterRadius > kBallOutlineThicknessWorld
                                              ? ballOuterRadius - kBallOutlineThicknessWorld
                                              : ballOuterRadius * 0.5f;
            ball.mesh = MeshFactory3D::makeDoubleSidedRing(
                ballInnerRadius,
                ballOuterRadius,
                kPlaneThicknessWorld,
                48,
                body->getColor());
            ball.material.solid.color = body->getColor();
            ball.material.wireframe.color = body->getColor();
            ball.material.renderSolid = true;
            ball.material.renderWireframe = false;
            ball.transform.position = toWorldPlanePosition(body->getPosition(), kSimulationPlaneZ);

            loadedScene->dynamicEntityIndices.push_back(loadedScene->getRenderScene().getEntities().size());
            loadedScene->getRenderScene().createEntity(ball);
        }
    }

    loadedScene->syncRenderScene();
    return loadedScene;
}

Scene2D &PhysicsRingScene::getPhysicsScene()
{
    return *physicsScene;
}

const Scene2D &PhysicsRingScene::getPhysicsScene() const
{
    return *physicsScene;
}

bool PhysicsRingScene::hasRenderScene() const
{
    return static_cast<bool>(renderScene);
}

Scene3D &PhysicsRingScene::getRenderScene()
{
    return *renderScene;
}

const Scene3D &PhysicsRingScene::getRenderScene() const
{
    return *renderScene;
}

PhysicsBody2D *PhysicsRingScene::getControlledBody()
{
    if (!physicsScene || controlledBodyIndex == kInvalidIndex || controlledBodyIndex >= physicsScene->getBodies().size())
    {
        return nullptr;
    }

    return physicsScene->getBodies()[controlledBodyIndex].get();
}

const PhysicsBody2D *PhysicsRingScene::getControlledBody() const
{
    if (!physicsScene || controlledBodyIndex == kInvalidIndex || controlledBodyIndex >= physicsScene->getBodies().size())
    {
        return nullptr;
    }

    return physicsScene->getBodies()[controlledBodyIndex].get();
}

void PhysicsRingScene::setWorldOffset(const Vector3 &offset)
{
    worldOffset = offset;
    syncRenderScene();
}

const Vector3 &PhysicsRingScene::getWorldOffset() const
{
    return worldOffset;
}

void PhysicsRingScene::syncRenderScene()
{
    if (!physicsScene || !renderScene)
    {
        return;
    }

    auto &bodies = physicsScene->getBodies();
    auto &entities = renderScene->getEntities();

    if (borderBodyIndex != kInvalidIndex &&
        borderEntityIndex != kInvalidIndex &&
        borderBodyIndex < bodies.size() &&
        borderEntityIndex < entities.size())
    {
        entities[borderEntityIndex].transform.position =
            toWorldPlanePosition(bodies[borderBodyIndex]->getPosition(), kSimulationPlaneZ) + worldOffset;
        entities[borderEntityIndex].transform.rotation = Vector3::zero();
        entities[borderEntityIndex].transform.scale = Vector3::one();
    }

    const std::size_t mappedCount = std::min(dynamicBodyIndices.size(), dynamicEntityIndices.size());
    for (std::size_t i = 0; i < mappedCount; ++i)
    {
        const std::size_t bodyIndex = dynamicBodyIndices[i];
        const std::size_t entityIndex = dynamicEntityIndices[i];
        if (bodyIndex >= bodies.size() || entityIndex >= entities.size())
        {
            continue;
        }

        entities[entityIndex].transform.position =
            toWorldPlanePosition(bodies[bodyIndex]->getPosition(), kSimulationPlaneZ) + worldOffset;
        entities[entityIndex].transform.rotation = Vector3::zero();
        entities[entityIndex].transform.scale = Vector3::one();
    }
}
