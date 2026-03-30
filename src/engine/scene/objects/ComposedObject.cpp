#include "engine/scene/objects/ComposedObject.h"

#include "engine/scene/3d/Entity.h"
#include "engine/scene/3d/Scene.h"

ComposedObject::~ComposedObject() = default;

std::unique_ptr<ComposedObject> ComposedObject::create(const ComposedObjectDesc &desc)
{
    auto object = std::make_unique<ComposedObject>();
    object->config = desc;
    object->worldOffset = desc.worldOffset;
    object->renderScene = std::make_unique<Scene>();

    for (const SceneBodyNodeDesc &node : desc.nodes)
    {
        NodeBinding binding;
        binding.name = node.name;
        binding.localPosition = node.render.localPosition;
        binding.localRotation = node.render.localRotation;
        binding.localScale = node.render.localScale;
        binding.renderEnabled = node.render.enabled;

        if (node.render.enabled)
        {
            Entity entity;
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

bool ComposedObject::isValid() const
{
    return static_cast<bool>(renderScene);
}

void ComposedObject::destroy()
{
    renderScene.reset();
    bindings.clear();
}

Scene &ComposedObject::getRenderScene()
{
    return *renderScene;
}

const Scene &ComposedObject::getRenderScene() const
{
    return *renderScene;
}

void ComposedObject::setWorldOffset(const Vector3 &offset)
{
    worldOffset = offset;
    syncRenderScene();
}

const Vector3 &ComposedObject::getWorldOffset() const
{
    return worldOffset;
}

void ComposedObject::syncRenderScene()
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

        Entity &entity = entities[binding.entityIndex];
        entity.transform.scale = binding.localScale;

        entity.transform.position = worldOffset + binding.localPosition;
        entity.transform.rotation = binding.localRotation;
    }
}
