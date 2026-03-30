#include "engine/scene/objects/ComposedObject3D.h"

#include <memory>
#include <utility>

#include "engine/physics/3d/PhysicsBody3D.h"
#include "engine/physics/3d/PhysicsWorld3D.h"
#include "engine/physics/3d/shapes/SphereShape3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/PhysicsScene3D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
std::unique_ptr<PhysicsBody3D> createPhysicsBody(const SceneBodyNodeDesc3D &node)
{
    if (!node.physics.enabled || !node.collider.enabled)
    {
        return nullptr;
    }

    switch (node.collider.shapeType)
    {
    case ColliderShape3DType::Sphere:
    {
        const float mass = node.physics.bodyType == PhysicsBodyType3D::Static ? 0.0f : node.physics.mass;
        return std::make_unique<PhysicsBody3D>(
            std::make_unique<SphereShape3D>(node.collider.sphereRadius),
            node.render.localPosition,
            node.render.localRotation,
            node.physics.startVelocity,
            mass,
            node.physics.surfaceMaterial,
            node.physics.rigidBodySettings);
    }
    case ColliderShape3DType::None:
    default:
        return nullptr;
    }
}
}

ComposedObject3D::~ComposedObject3D() = default;

std::unique_ptr<ComposedObject3D> ComposedObject3D::create(const ComposedObject3DDesc &desc)
{
    auto object = std::make_unique<ComposedObject3D>();
    object->config = desc;
    object->worldOffset = desc.worldOffset;
    object->physicsScene = std::make_unique<PhysicsScene3D>();
    object->renderScene = std::make_unique<Scene3D>();

    for (const SceneBodyNodeDesc3D &node : desc.nodes)
    {
        NodeBinding binding;
        binding.name = node.name;
        binding.localPosition = node.render.localPosition;
        binding.localRotation = node.render.localRotation;
        binding.localScale = node.render.localScale;
        binding.renderEnabled = node.render.enabled;
        binding.physicsEnabled = node.physics.enabled && node.collider.enabled;
        binding.syncRotationFromPhysics = node.physics.syncRotationFromPhysics;

        if (std::unique_ptr<PhysicsBody3D> body = createPhysicsBody(node))
        {
            binding.bodyIndex = object->physicsScene->getBodies().size();
            object->physicsScene->addBody(std::move(body));
        }

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
    return static_cast<bool>(physicsScene) && static_cast<bool>(renderScene);
}

void ComposedObject3D::destroy()
{
    physicsScene.reset();
    renderScene.reset();
    bindings.clear();
}

PhysicsScene3D &ComposedObject3D::getPhysicsScene()
{
    return *physicsScene;
}

const PhysicsScene3D &ComposedObject3D::getPhysicsScene() const
{
    return *physicsScene;
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

void ComposedObject3D::step(PhysicsWorld3D &physicsWorld, float dt)
{
    if (!physicsScene)
    {
        return;
    }

    if (!physicsScene->hasAwakeDynamicBodies())
    {
        return;
    }

    physicsWorld.step(*physicsScene, dt);
    syncRenderScene();
}

void ComposedObject3D::syncRenderScene()
{
    if (!isValid())
    {
        return;
    }

    auto &bodies = physicsScene->getBodies();
    auto &entities = renderScene->getEntities();

    for (const NodeBinding &binding : bindings)
    {
        if (!binding.renderEnabled || binding.entityIndex >= entities.size())
        {
            continue;
        }

        Entity3D &entity = entities[binding.entityIndex];
        entity.transform.scale = binding.localScale;

        if (binding.physicsEnabled && binding.bodyIndex < bodies.size())
        {
            const PhysicsBody3D &body = *bodies[binding.bodyIndex];
            entity.transform.position = worldOffset + body.getPosition();
            if (binding.syncRotationFromPhysics)
            {
                entity.transform.rotation = Vector3::zero();
                entity.transform.setCustomRotationMatrix(body.getRotationMatrix());
            }
            else
            {
                entity.transform.rotation = body.getRotation();
                entity.transform.clearCustomRotationMatrix();
            }
            continue;
        }

        entity.transform.position = worldOffset + binding.localPosition;
        entity.transform.rotation = binding.localRotation;
        entity.transform.clearCustomRotationMatrix();
    }
}
