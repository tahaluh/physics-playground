#include "engine/scene/objects/BodyObject.h"

#include <memory>

#include "engine/physics/BoxCollider.h"
#include "engine/render/3d/mesh/MeshFactory.h"
#include "engine/scene/3d/Entity.h"
#include "engine/scene/3d/Scene.h"

namespace
{
std::shared_ptr<Collider> makeDefaultCollider(const BodyObjectDesc &desc)
{
    switch (desc.shapeType)
    {
    case BodyShapeType::Cube:
        return std::make_shared<BoxCollider>(Vector3::zero(), Vector3::one() * 0.5f);
    case BodyShapeType::Sphere:
    default:
        return std::make_shared<BoxCollider>(Vector3::zero(), Vector3::one() * 0.5f);
    }
}

Mesh makeBodyMesh(const BodyObjectDesc &desc)
{
    switch (desc.shapeType)
    {
    case BodyShapeType::Sphere:
        return MeshFactory::makeSphere(0.5f, desc.sphereRings, desc.sphereSegments, 0);
    case BodyShapeType::Cube:
    default:
        return MeshFactory::makeCube(1.0f);
    }
}
}

BodyObject::~BodyObject() = default;

std::unique_ptr<BodyObject> BodyObject::create(const BodyObjectDesc &desc)
{
    auto object = std::make_unique<BodyObject>();
    object->config = desc;
    object->config.collider = desc.collider ? desc.collider->clone() : makeDefaultCollider(desc);
    object->physicsState = desc.physics;
    object->renderScene = std::make_unique<Scene>();

    if (desc.renderEnabled)
    {
        Entity entity;
        entity.name = desc.name;
        switch (desc.shapeType)
        {
        case BodyShapeType::Sphere:
            entity.instancingKey = "body:sphere:" + std::to_string(desc.sphereRings) + ":" + std::to_string(desc.sphereSegments);
            break;
        case BodyShapeType::Cube:
        default:
            entity.instancingKey = "body:cube";
            break;
        }
        entity.supportsInstancing = true;
        entity.simulateOnGpu = desc.simulateOnGpu;
        entity.linearVelocity = desc.physics.linearVelocity;
        entity.angularVelocity = desc.physics.angularVelocity;
        entity.mesh = makeBodyMesh(desc);
        entity.material = desc.material;
        entity.transform = desc.transform;
        object->entityIndex = object->renderScene->getEntities().size();
        object->renderScene->createEntity(entity);
    }

    object->syncRenderScene();
    return object;
}

bool BodyObject::isValid() const
{
    return static_cast<bool>(renderScene);
}

void BodyObject::destroy()
{
    renderScene.reset();
    entityIndex = static_cast<std::size_t>(-1);
}

Scene &BodyObject::getRenderScene()
{
    return *renderScene;
}

const Scene &BodyObject::getRenderScene() const
{
    return *renderScene;
}

const BodyObjectDesc &BodyObject::getConfig() const
{
    return config;
}

const Collider *BodyObject::getCollider() const
{
    return config.collider.get();
}

const Material &BodyObject::getMaterial() const
{
    return config.material;
}

const BodyPhysicsState &BodyObject::getPhysicsState() const
{
    return physicsState;
}

void BodyObject::setPhysicsState(const BodyPhysicsState &state)
{
    physicsState = state;
    config.physics = state;
}

bool BodyObject::isSleeping() const
{
    return physicsState.sleeping;
}

void BodyObject::setSleeping(bool sleeping)
{
    if (physicsState.sleeping == sleeping)
    {
        return;
    }

    physicsState.sleeping = sleeping;
    if (!sleeping)
    {
        physicsState.sleepTime = 0.0f;
    }
    config.physics = physicsState;
}

void BodyObject::wakeUp()
{
    if (!physicsState.sleeping && physicsState.sleepTime == 0.0f)
    {
        return;
    }

    physicsState.sleeping = false;
    physicsState.sleepTime = 0.0f;
    config.physics = physicsState;
}

void BodyObject::setCollider(const std::shared_ptr<Collider> &collider)
{
    config.collider = collider ? collider->clone() : nullptr;
}

void BodyObject::setMaterial(const Material &material)
{
    config.material = material;
    syncRenderScene();
}

void BodyObject::setTransform(const Transform &transform)
{
    config.transform = transform;
    syncRenderScene();
}

const Transform &BodyObject::getTransform() const
{
    return config.transform;
}

void BodyObject::notifyCollisionEnter(BodyObject &other, const CollisionPoints &collision)
{
    if (onCollisionEnter)
    {
        onCollisionEnter(*this, other, collision);
    }
}

void BodyObject::notifyCollisionStay(BodyObject &other, const CollisionPoints &collision)
{
    if (onCollision)
    {
        onCollision(*this, other, collision);
    }
}

void BodyObject::notifyCollisionExit(BodyObject &other, const CollisionPoints &collision)
{
    if (onCollisionExit)
    {
        onCollisionExit(*this, other, collision);
    }
}

void BodyObject::syncRenderScene()
{
    if (!isValid())
    {
        return;
    }

    auto &entities = renderScene->getEntities();
    if (entityIndex >= entities.size())
    {
        return;
    }

    entities[entityIndex].transform = config.transform;
    entities[entityIndex].linearVelocity = physicsState.linearVelocity;
    entities[entityIndex].angularVelocity = physicsState.angularVelocity;
    entities[entityIndex].simulateOnGpu = config.simulateOnGpu;
    entities[entityIndex].material = config.material;
}
