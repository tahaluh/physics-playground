#include "engine/scene/objects/ComposedObject3D.h"

#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/Scene3D.h"

ComposedObject3D::~ComposedObject3D() = default;

std::unique_ptr<ComposedObject3D> ComposedObject3D::create(const ComposedObject3DDesc &desc)
{
    auto object = std::make_unique<ComposedObject3D>();
    object->config = desc;
    object->worldOffset = desc.worldOffset;
    object->renderScene = std::make_unique<Scene3D>();

    for (const SceneBodyNodeDesc3D &node : desc.nodes)
    {
        NodeBinding binding;
        binding.name = node.name;
        binding.localPosition = node.render.localPosition;
        binding.localRotation = node.render.localRotation;
        binding.localScale = node.render.localScale;
        binding.renderEnabled = node.render.enabled;

        if (node.render.enabled)
        {
            Entity3D entity;
            entity.name = node.name;
            entity.mesh = node.render.mesh;
            entity.material = node.render.material;
            entity.transform.position = object->worldOffset + node.render.localPosition;
            entity.transform.rotation = node.render.localRotation;
            entity.transform.scale = node.render.localScale;
            binding.entityIndex = object->renderScene->getEntities().size();
            object->renderScene->createEntity(entity);
        }

        object->bindings.push_back(binding);
    }

    object->syncRenderScene();
    return object;
}

bool ComposedObject3D::isValid() const
{
    return static_cast<bool>(renderScene);
}

void ComposedObject3D::destroy()
{
    renderScene.reset();
    bindings.clear();
}

Scene3D &ComposedObject3D::getRenderScene()
{
    return *renderScene;
}

const Scene3D &ComposedObject3D::getRenderScene() const
{
    return *renderScene;
}

void ComposedObject3D::setWorldOffset(const Vector3 &offset)
{
    worldOffset = offset;
    syncRenderScene();
}

const Vector3 &ComposedObject3D::getWorldOffset() const
{
    return worldOffset;
}

void ComposedObject3D::syncRenderScene()
{
    if (!isValid())
    {
        return;
    }

    auto &entities = renderScene->getEntities();

    for (const NodeBinding &binding : bindings)
    {
        if (!binding.renderEnabled || binding.entityIndex >= entities.size())
        {
            continue;
        }

        Entity3D &entity = entities[binding.entityIndex];
        entity.transform.scale = binding.localScale;

        entity.transform.position = worldOffset + binding.localPosition;
        entity.transform.rotation = binding.localRotation;
        entity.transform.clearCustomRotationMatrix();
    }
}
