#include "engine/scene/objects/BodyObject3D.h"

#include <memory>

#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
Mesh3D makeBodyMesh(const BodyObject3DDesc &desc)
{
    switch (desc.shapeType)
    {
    case BodyShapeType3D::Sphere:
        return MeshFactory3D::makeSphere(0.5f, desc.sphereRings, desc.sphereSegments, 0);
    case BodyShapeType3D::Cube:
    default:
        return MeshFactory3D::makeCube(1.0f);
    }
}
}

BodyObject3D::~BodyObject3D() = default;

std::unique_ptr<BodyObject3D> BodyObject3D::create(const BodyObject3DDesc &desc)
{
    auto object = std::make_unique<BodyObject3D>();
    object->config = desc;
    object->renderScene = std::make_unique<Scene3D>();

    if (desc.renderEnabled)
    {
        Entity3D entity;
        entity.name = desc.name;
        switch (desc.shapeType)
        {
        case BodyShapeType3D::Sphere:
            entity.instancingKey = "body:sphere:" + std::to_string(desc.sphereRings) + ":" + std::to_string(desc.sphereSegments);
            break;
        case BodyShapeType3D::Cube:
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

bool BodyObject3D::isValid() const
{
    return static_cast<bool>(renderScene);
}

void BodyObject3D::destroy()
{
    renderScene.reset();
    entityIndex = static_cast<std::size_t>(-1);
}

Scene3D &BodyObject3D::getRenderScene()
{
    return *renderScene;
}

const Scene3D &BodyObject3D::getRenderScene() const
{
    return *renderScene;
}

const BodyObject3DDesc &BodyObject3D::getConfig() const
{
    return config;
}

void BodyObject3D::setTransform(const Transform3D &transform)
{
    config.transform = transform;
    syncRenderScene();
}

const Transform3D &BodyObject3D::getTransform() const
{
    return config.transform;
}

void BodyObject3D::syncRenderScene()
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
