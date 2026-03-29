#include "engine/scene/objects/RingObject.h"

#include <algorithm>
#include <memory>

#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"
#include "engine/physics/2d/BallBody2D.h"
#include "engine/physics/2d/BorderCircleBody2D.h"
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/PhysicsWorld2D.h"
#include "engine/physics/2d/shapes/CircleShape.h"
#include "engine/render/2d/Object2DIn3D.h"
#include "engine/scene/2d/Scene2D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
Vector3 toWorldPosition(const RingObjectDesc &config, const Vector2 &simulationPosition, float zOffset = 0.0f)
{
    const float worldX = (simulationPosition.x - config.center.x) / config.simulationScale;
    const float worldY = -(simulationPosition.y - config.center.y) / config.simulationScale;
    return Vector3(worldX, worldY, zOffset);
}

float toWorldRadius(const RingObjectDesc &config, float simulationRadius)
{
    return simulationRadius / config.simulationScale;
}
}

RingObject::~RingObject() = default;

RingObjectDesc RingObject::makeDefaultDesc()
{
    RingObjectDesc desc;
    desc.borderMaterial.solid.baseColor = desc.borderColor;
    desc.borderMaterial.wireframe.baseColor = desc.borderColor;
    desc.borderMaterial.renderSolid = true;
    desc.borderMaterial.renderWireframe = false;
    desc.ballMaterial.solid.baseColor = desc.ballColor;
    desc.ballMaterial.wireframe.baseColor = desc.ballColor;
    desc.ballMaterial.renderSolid = true;
    desc.ballMaterial.renderWireframe = false;
    return desc;
}

std::unique_ptr<RingObject> RingObject::create(const RingObjectDesc &desc)
{
    auto object = std::make_unique<RingObject>();
    object->config = desc;
    object->worldOffset = desc.worldOffset;
    object->physicsScene = std::make_unique<Scene2D>();
    object->renderScene = std::make_unique<Scene3D>();
    object->renderScene->getAmbientLight().color = 0xFFFFFFFF;
    object->renderScene->getAmbientLight().intensity = 1.0f;

    object->borderBodyIndex = object->physicsScene->getBodies().size();
    object->physicsScene->addBody(std::make_unique<BorderCircleBody2D>(desc.center, desc.borderRadiusPixels));

    object->controlledBodyIndex = object->physicsScene->getBodies().size();
    auto ballBody = std::make_unique<BallBody2D>(
        desc.ballRadiusPixels,
        desc.ballStartPosition,
        desc.ballStartVelocity,
        desc.ballColor);
    ballBody->getMaterial() = desc.physicsMaterial;
    object->physicsScene->addBody(std::move(ballBody));

    Shape2DIn3DDesc borderDesc;
    borderDesc.kind = Shape2DIn3DKind::Ring;
    borderDesc.name = "PhysicsBorder";
    borderDesc.position = Vector2(
        toWorldPosition(desc, desc.center, desc.planeZ).x,
        toWorldPosition(desc, desc.center, desc.planeZ).y);
    borderDesc.planeZ = desc.planeZ;
    borderDesc.radius = toWorldRadius(desc, desc.borderRadiusPixels);
    borderDesc.innerRadius = borderDesc.radius - desc.borderThicknessWorld;
    borderDesc.segments = desc.borderSegments;
    borderDesc.color = desc.borderColor;
    borderDesc.material = desc.borderMaterial;
    object->borderEntityIndex = object->renderScene->getEntities().size();
    object->renderScene->createEntity(makeShape2DIn3D(borderDesc));

    const auto &bodies = object->physicsScene->getBodies();
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

        object->dynamicBodyIndices.push_back(bodyIndex);

        Shape2DIn3DDesc ballDesc;
        ballDesc.kind = Shape2DIn3DKind::Disc;
        ballDesc.name = "PhysicsBall";
        ballDesc.position = Vector2(
            toWorldPosition(desc, body->getPosition(), desc.planeZ).x,
            toWorldPosition(desc, body->getPosition(), desc.planeZ).y);
        ballDesc.planeZ = desc.planeZ;
        ballDesc.radius = circle->getRadius() / desc.simulationScale;
        ballDesc.segments = desc.ballSegments;
        ballDesc.color = body->getColor();
        ballDesc.material = desc.ballMaterial;
        ballDesc.material.solid.baseColor = body->getColor();
        ballDesc.material.wireframe.baseColor = body->getColor();

        object->dynamicEntityIndices.push_back(object->renderScene->getEntities().size());
        object->renderScene->createEntity(makeShape2DIn3D(ballDesc));
    }

    object->syncRenderScene();
    return object;
}

std::unique_ptr<RingObject> RingObject::createDefault()
{
    return create(makeDefaultDesc());
}

void RingObject::destroy()
{
    physicsScene.reset();
    renderScene.reset();
    controlledBodyIndex = kInvalidIndex;
    borderBodyIndex = kInvalidIndex;
    borderEntityIndex = kInvalidIndex;
    dynamicBodyIndices.clear();
    dynamicEntityIndices.clear();
}

bool RingObject::isValid() const
{
    return static_cast<bool>(physicsScene) && static_cast<bool>(renderScene);
}

Scene2D &RingObject::getPhysicsScene() { return *physicsScene; }
const Scene2D &RingObject::getPhysicsScene() const { return *physicsScene; }
Scene3D &RingObject::getRenderScene() { return *renderScene; }
const Scene3D &RingObject::getRenderScene() const { return *renderScene; }

PhysicsBody2D *RingObject::getControlledBody()
{
    if (!isValid() || controlledBodyIndex == kInvalidIndex || controlledBodyIndex >= physicsScene->getBodies().size())
        return nullptr;
    return physicsScene->getBodies()[controlledBodyIndex].get();
}

const PhysicsBody2D *RingObject::getControlledBody() const
{
    if (!isValid() || controlledBodyIndex == kInvalidIndex || controlledBodyIndex >= physicsScene->getBodies().size())
        return nullptr;
    return physicsScene->getBodies()[controlledBodyIndex].get();
}

void RingObject::setWorldOffset(const Vector3 &offset)
{
    worldOffset = offset;
    syncRenderScene();
}

const Vector3 &RingObject::getWorldOffset() const { return worldOffset; }

void RingObject::step(PhysicsWorld2D &physicsWorld, float dt)
{
    if (!physicsScene)
        return;
    physicsWorld.step(*physicsScene, dt);
    syncRenderScene();
}

void RingObject::syncRenderScene()
{
    if (!isValid())
        return;

    auto &bodies = physicsScene->getBodies();
    auto &entities = renderScene->getEntities();

    if (borderBodyIndex < bodies.size() && borderEntityIndex < entities.size())
    {
        entities[borderEntityIndex].transform.position =
            toWorldPosition(config, bodies[borderBodyIndex]->getPosition(), config.planeZ) + worldOffset;
        entities[borderEntityIndex].transform.rotation = Vector3(0.0f, 0.0f, bodies[borderBodyIndex]->getRotationAngle());
        entities[borderEntityIndex].transform.scale = Vector3::one();
    }

    const std::size_t mappedCount = std::min(dynamicBodyIndices.size(), dynamicEntityIndices.size());
    for (std::size_t i = 0; i < mappedCount; ++i)
    {
        const std::size_t bodyIndex = dynamicBodyIndices[i];
        const std::size_t entityIndex = dynamicEntityIndices[i];
        if (bodyIndex >= bodies.size() || entityIndex >= entities.size())
            continue;

        entities[entityIndex].transform.position =
            toWorldPosition(config, bodies[bodyIndex]->getPosition(), config.planeZ) + worldOffset;
        entities[entityIndex].transform.rotation = Vector3(0.0f, 0.0f, bodies[bodyIndex]->getRotationAngle());
        entities[entityIndex].transform.scale = Vector3::one();
    }
}
