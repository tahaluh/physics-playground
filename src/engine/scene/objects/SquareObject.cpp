#include "engine/scene/objects/SquareObject.h"

#include <memory>

#include "engine/scene/objects/RingObject.h"
#include "engine/physics/2d/BoxBody2D.h"
#include "engine/physics/2d/PhysicsBody2D.h"
#include "engine/physics/2d/shapes/RectShape.h"
#include "engine/render/2d/Object2DIn3D.h"
#include "engine/scene/2d/Scene2D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
Vector3 toWorldPosition(const RingObjectDesc &ringConfig, const Vector2 &simulationPosition, float zOffset = 0.0f)
{
    const float worldX = (simulationPosition.x - ringConfig.center.x) / ringConfig.simulationScale;
    const float worldY = -(simulationPosition.y - ringConfig.center.y) / ringConfig.simulationScale;
    return Vector3(worldX, worldY, zOffset);
}

float toWorldRotation(float simulationRotation)
{
    return -simulationRotation;
}
}

SquareObject::~SquareObject() = default;

SquareObjectDesc SquareObject::makeDefaultDesc()
{
    SquareObjectDesc desc;
    desc.material.solid.baseColor = desc.color;
    desc.material.wireframe.baseColor = desc.color;
    desc.material.renderSolid = true;
    desc.material.renderWireframe = false;
    return desc;
}

std::unique_ptr<SquareObject> SquareObject::create(const SquareObjectDesc &desc, Scene2D &physicsScene, const RingObjectDesc &ringConfig)
{
    auto object = std::make_unique<SquareObject>();
    object->config = desc;
    object->renderScene = std::make_unique<Scene3D>();
    object->renderScene->getAmbientLight().color = 0xFFFFFFFF;
    object->renderScene->getAmbientLight().intensity = 1.0f;

    object->bodyIndex = physicsScene.getBodies().size();
    auto squareBody = std::make_unique<BoxBody2D>(
        desc.sizePixels,
        desc.sizePixels,
        desc.startPosition,
        desc.startVelocity,
        desc.color);
    squareBody->getSurfaceMaterial() = desc.physicsSurfaceMaterial;
    squareBody->getRigidBodySettings() = desc.rigidBodySettings;
    squareBody->setCollisionEnabled(false);
    physicsScene.addBody(std::move(squareBody));

    Shape2DIn3DDesc squareDesc;
    squareDesc.kind = Shape2DIn3DKind::Rectangle;
    squareDesc.name = "Square";
    squareDesc.position = Vector2(
        toWorldPosition(ringConfig, desc.startPosition, ringConfig.planeZ).x,
        toWorldPosition(ringConfig, desc.startPosition, ringConfig.planeZ).y);
    squareDesc.planeZ = ringConfig.planeZ + 0.0003f;
    squareDesc.size = Vector2(
        desc.sizePixels / ringConfig.simulationScale,
        desc.sizePixels / ringConfig.simulationScale);
    squareDesc.color = desc.color;
    squareDesc.material = desc.material;
    object->squareEntityIndex = object->renderScene->getEntities().size();
    object->renderScene->createEntity(makeShape2DIn3D(squareDesc));

    object->syncRenderScene(physicsScene, ringConfig);
    return object;
}

void SquareObject::destroy()
{
    renderScene.reset();
    bodyIndex = kInvalidIndex;
    squareEntityIndex = kInvalidIndex;
}

bool SquareObject::isValid() const
{
    return static_cast<bool>(renderScene) && bodyIndex != kInvalidIndex;
}

Scene3D &SquareObject::getRenderScene() { return *renderScene; }
const Scene3D &SquareObject::getRenderScene() const { return *renderScene; }

void SquareObject::syncRenderScene(const Scene2D &physicsScene, const RingObjectDesc &ringConfig)
{
    if (!renderScene)
    {
        return;
    }

    const auto &bodies = physicsScene.getBodies();
    auto &entities = renderScene->getEntities();
    if (bodyIndex >= bodies.size() || squareEntityIndex >= entities.size())
    {
        return;
    }

    const PhysicsBody2D &body = *bodies[bodyIndex];
    Vector2 bodyPosition = body.getPosition();
    if (const auto *rect = dynamic_cast<const RectShape *>(body.getShape()))
    {
        bodyPosition += Vector2(rect->getWidth() * 0.5f, rect->getHeight() * 0.5f);
    }

    entities[squareEntityIndex].transform.position =
        toWorldPosition(ringConfig, bodyPosition, ringConfig.planeZ + 0.0003f) + ringConfig.worldOffset;
    entities[squareEntityIndex].transform.rotation = Vector3(0.0f, 0.0f, toWorldRotation(body.getRotationAngle()));
    entities[squareEntityIndex].transform.scale = Vector3::one();
}
