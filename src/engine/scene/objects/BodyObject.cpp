#include "engine/scene/objects/BodyObject.h"

#include <memory>

#include "engine/render/3d/mesh/MeshFactory.h"
#include "engine/scene/3d/Entity.h"
#include "engine/scene/3d/Scene.h"

namespace
{
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

const BodyPhysicsState &BodyObject::getPhysicsState() const
{
    return physicsState;
}

void BodyObject::setPhysicsState(const BodyPhysicsState &state)
{
    physicsState = state;
    config.physics = state;
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
    entities[entityIndex].material = config.material;
}
