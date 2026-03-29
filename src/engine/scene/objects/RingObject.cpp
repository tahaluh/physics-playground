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
#include "engine/physics/2d/shapes/RectShape.h"
#include "engine/render/2d/Object2DIn3D.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/2d/Scene2D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
constexpr float kCenterOfMassMarkerRadius = 0.06f;

Mesh3D makeRadialLineMesh()
{
    Mesh3D mesh;
    mesh.vertices = {
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(1.0f, 0.0f, 0.0f)};
    mesh.vertexNormals = {
        Vector3(0.0f, 0.0f, 1.0f),
        Vector3(0.0f, 0.0f, 1.0f)};
    mesh.edges = {{0, 1}};
    return mesh;
}

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

float toWorldRotation(float simulationRotation)
{
    return -simulationRotation;
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
    object->physicsScene->addBody(std::make_unique<BorderCircleBody2D>(
        desc.center,
        std::max(0.0f, desc.borderRadiusPixels - desc.borderThicknessPixels)));

    object->controlledBodyIndex = object->physicsScene->getBodies().size();
    auto ballBody = std::make_unique<BallBody2D>(
        desc.ballRadiusPixels,
        desc.ballStartPosition,
        desc.ballStartVelocity,
        desc.ballColor);
    ballBody->getSurfaceMaterial() = desc.physicsSurfaceMaterial;
    ballBody->getRigidBodySettings() = desc.rigidBodySettings;
    object->physicsScene->addBody(std::move(ballBody));

    Shape2DIn3DDesc borderDesc;
    borderDesc.kind = Shape2DIn3DKind::Ring;
    borderDesc.name = "PhysicsBorder";
    borderDesc.position = Vector2(
        toWorldPosition(desc, desc.center, desc.planeZ).x,
        toWorldPosition(desc, desc.center, desc.planeZ).y);
    borderDesc.planeZ = desc.planeZ;
    borderDesc.radius = toWorldRadius(desc, desc.borderRadiusPixels);
    borderDesc.innerRadius = std::max(0.0f, borderDesc.radius - toWorldRadius(desc, desc.borderThicknessPixels));
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

        RingDynamicBodyVisualBinding binding;
        binding.bodyIndex = bodyIndex;

        if (circle)
        {
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
            binding.entityIndex = object->renderScene->getEntities().size();
            object->renderScene->createEntity(makeShape2DIn3D(ballDesc));
            binding.rotationIndicatorOffset = 0.0f;
            binding.rotationIndicatorScale = Vector3(ballDesc.radius, 1.0f, 1.0f);
        }
        if (desc.showRotationIndicators && circle)
        {
            Entity3D indicator;
            indicator.name = "RotationIndicator";
            indicator.mesh = makeRadialLineMesh();
            indicator.material.solid.baseColor = desc.rotationIndicatorColor;
            indicator.material.wireframe.baseColor = desc.rotationIndicatorColor;
            indicator.material.renderSolid = false;
            indicator.material.renderWireframe = true;
            indicator.material.wireframe.doubleSidedLighting = true;
            binding.rotationIndicatorEntityIndex = object->renderScene->getEntities().size();
            object->renderScene->createEntity(indicator);
        }

        object->dynamicBindings.push_back(binding);
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
    dynamicBindings.clear();
}

bool RingObject::isValid() const
{
    return static_cast<bool>(physicsScene) && static_cast<bool>(renderScene);
}

Scene2D &RingObject::getPhysicsScene() { return *physicsScene; }
const Scene2D &RingObject::getPhysicsScene() const { return *physicsScene; }
Scene3D &RingObject::getRenderScene() { return *renderScene; }
const Scene3D &RingObject::getRenderScene() const { return *renderScene; }
const RingObjectDesc &RingObject::getConfig() const { return config; }

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
        entities[borderEntityIndex].transform.rotation = Vector3(0.0f, 0.0f, toWorldRotation(bodies[borderBodyIndex]->getRotationAngle()));
        entities[borderEntityIndex].transform.scale = Vector3::one();
    }

    for (const RingDynamicBodyVisualBinding &binding : dynamicBindings)
    {
        const std::size_t bodyIndex = binding.bodyIndex;
        const std::size_t entityIndex = binding.entityIndex;
        if (bodyIndex >= bodies.size() || entityIndex >= entities.size())
            continue;

        PhysicsBody2D &body = *bodies[bodyIndex];
        Vector2 bodyPosition = body.getPosition();
        if (const auto *rect = dynamic_cast<const RectShape *>(body.getShape()))
        {
            bodyPosition += Vector2(rect->getWidth() * 0.5f, rect->getHeight() * 0.5f);
        }

        entities[entityIndex].transform.position =
            toWorldPosition(config, bodyPosition, config.planeZ) + worldOffset;
        entities[entityIndex].transform.rotation = Vector3(0.0f, 0.0f, toWorldRotation(body.getRotationAngle()));
        entities[entityIndex].transform.scale = binding.usesCustomScale ? binding.scale : Vector3::one();

        if (binding.rotationIndicatorEntityIndex != kInvalidIndex &&
            binding.rotationIndicatorEntityIndex < entities.size())
        {
            entities[binding.rotationIndicatorEntityIndex].transform.position =
                toWorldPosition(config, bodyPosition, config.planeZ + 0.0005f) + worldOffset;
            entities[binding.rotationIndicatorEntityIndex].transform.rotation =
                Vector3(0.0f, 0.0f, toWorldRotation(body.getRotationAngle()));
            entities[binding.rotationIndicatorEntityIndex].transform.scale = binding.rotationIndicatorScale;
        }
    }
}

void RingObject::appendDebugMarkers(Scene3D &targetScene) const
{
    if (!physicsScene)
    {
        return;
    }

    for (const auto &bodyPtr : physicsScene->getBodies())
    {
        if (!bodyPtr)
        {
            continue;
        }

        const PhysicsBody2D &body = *bodyPtr;
        Vector2 centerOfMass = body.getPosition();
        if (const auto *rect = dynamic_cast<const RectShape *>(body.getShape()))
        {
            centerOfMass += Vector2(rect->getWidth() * 0.5f, rect->getHeight() * 0.5f);
        }

        Shape2DIn3DDesc markerDesc;
        markerDesc.kind = Shape2DIn3DKind::Disc;
        markerDesc.name = body.isStatic() ? "StaticBodyCenterDebug2D" : "DynamicBodyCenterDebug2D";
        markerDesc.position = Vector2(
            toWorldPosition(config, centerOfMass, config.planeZ + 0.001f).x,
            toWorldPosition(config, centerOfMass, config.planeZ + 0.001f).y);
        markerDesc.planeZ = config.planeZ + 0.001f;
        markerDesc.radius = body.isStatic() ? kCenterOfMassMarkerRadius * 0.8f : kCenterOfMassMarkerRadius;
        markerDesc.segments = 20;
        markerDesc.color = body.isStatic() ? 0xFFFFB347 : 0xFF59E3FF;
        markerDesc.material.renderSolid = true;
        markerDesc.material.renderWireframe = false;
        markerDesc.material.solid.baseColor = markerDesc.color;
        markerDesc.material.solid.opacity = 0.9f;
        markerDesc.material.solid.doubleSidedLighting = true;

        Entity3D marker = makeShape2DIn3D(markerDesc);
        marker.transform.position += worldOffset;
        targetScene.createEntity(marker);
    }
}
