#include "engine/scene/3d/RuntimeScene.h"

#include <algorithm>
#include <utility>

#include "engine/render/3d/mesh/MeshFactory.h"

namespace
{
    Entity makeBodyEntity(const BodyObjectDesc &desc)
    {
        Entity entity;
        entity.name = desc.name;
        entity.supportsInstancing = true;
        entity.linearVelocity = desc.linearVelocity;
        entity.angularVelocity = desc.angularVelocity;
        entity.material = desc.material;
        entity.transform = desc.transform;

        switch (desc.shapeType)
        {
        case BodyShapeType::Plane:
            entity.instancingKey = "body:plane";
            entity.mesh = MeshFactory::makeQuadXZ(1.0f);
            break;
        case BodyShapeType::Sphere:
            entity.instancingKey = "body:sphere:" + std::to_string(desc.sphere.rings) + ":" + std::to_string(desc.sphere.segments);
            entity.mesh = MeshFactory::makeSphere(0.5f, desc.sphere.rings, desc.sphere.segments, 0);
            break;
        case BodyShapeType::Cube:
        default:
            entity.instancingKey = "body:cube";
            entity.mesh = MeshFactory::makeCube(1.0f);
            break;
        }

        return entity;
    }
}

BodyObject &RuntimeScene::addBody(const BodyObjectDesc &desc)
{
    return addBody(BodyObject::create(desc));
}

BodyObject &RuntimeScene::addBody(std::unique_ptr<BodyObject> body)
{
    bodies.push_back(std::move(body));
    BodyObject &addedBody = *bodies.back();
    physicsSystem.addBody(addedBody);
    appendBodyRenderScene(addedBody);
    renderScene.touch();
    return addedBody;
}

void RuntimeScene::removeBody(const BodyObject &body)
{
    const auto it = std::find_if(bodies.begin(), bodies.end(), [&body](const std::unique_ptr<BodyObject> &candidate)
                                 { return candidate.get() == &body; });
    if (it == bodies.end())
    {
        return;
    }

    physicsSystem.removeBody(body);
    bodies.erase(it);
    rebuildBodyRenderScene();
}

void RuntimeScene::clearBodies()
{
    bodies.clear();
    bodyRanges.clear();
    physicsSystem.clearBodies();
    renderScene.clearEntities();
}

void RuntimeScene::step(float dt)
{
    physicsSystem.step(dt);
    syncBodyRenderScenes();
}

void RuntimeScene::setWireframeVisible(bool visible)
{
    wireframeVisible = visible;
    renderScene.applyWireframeVisibilityOverride(wireframeVisible);
}

Scene &RuntimeScene::getRenderScene()
{
    return renderScene;
}

const Scene &RuntimeScene::getRenderScene() const
{
    return renderScene;
}

std::vector<std::unique_ptr<BodyObject>> &RuntimeScene::getBodies()
{
    return bodies;
}

const std::vector<std::unique_ptr<BodyObject>> &RuntimeScene::getBodies() const
{
    return bodies;
}

void RuntimeScene::appendBodyRenderScene(const BodyObject &body)
{
    SceneEntityRange range;
    range.start = renderScene.getEntities().size();
    if (!body.getConfig().renderEnabled)
    {
        range.count = 0;
        bodyRanges.push_back(range);
        return;
    }

    renderScene.createEntity(makeBodyEntity(body.getConfig()));
    renderScene.applyWireframeVisibilityOverride(wireframeVisible);
    range.count = 1;
    bodyRanges.push_back(range);
}

void RuntimeScene::syncBodyRenderScenes()
{
    std::vector<Entity> &targetEntities = renderScene.getEntities();
    bool anyTransformChanged = false;
    bool anyMaterialChanged = false;

    for (std::size_t i = 0; i < bodies.size() && i < bodyRanges.size(); ++i)
    {
        BodyObject *body = bodies[i].get();
        if (!body)
        {
            continue;
        }

        const BodyObject::RenderSyncChange syncChange = body->syncRenderScene();
        if (syncChange == BodyObject::RenderSyncChange::None)
        {
            continue;
        }

        anyTransformChanged = anyTransformChanged || (static_cast<int>(syncChange) & static_cast<int>(BodyObject::RenderSyncChange::Transform));
        anyMaterialChanged = anyMaterialChanged || (static_cast<int>(syncChange) & static_cast<int>(BodyObject::RenderSyncChange::Material));

        const SceneEntityRange &range = bodyRanges[i];
        if (range.count == 0 || range.start + range.count > targetEntities.size())
        {
            continue;
        }

        const BodyObjectDesc &config = body->getConfig();
        for (std::size_t entityIndex = 0; entityIndex < range.count; ++entityIndex)
        {
            Entity &targetEntity = targetEntities[range.start + entityIndex];

            if (static_cast<int>(syncChange) & static_cast<int>(BodyObject::RenderSyncChange::Transform))
            {
                targetEntity.transform = config.transform;
            }
            if (static_cast<int>(syncChange) & static_cast<int>(BodyObject::RenderSyncChange::Material))
            {
                targetEntity.material = config.material;
            }

            targetEntity.material.renderWireframe = wireframeVisible;
        }
    }

    if (anyMaterialChanged)
    {
        renderScene.touchMaterials();
    }
    if (anyTransformChanged)
    {
        renderScene.touchTransforms();
    }
}

void RuntimeScene::rebuildBodyRenderScene()
{
    renderScene.clearEntities();
    bodyRanges.clear();
    for (const std::unique_ptr<BodyObject> &body : bodies)
    {
        if (!body)
        {
            bodyRanges.push_back({});
            continue;
        }

        appendBodyRenderScene(*body);
    }

    renderScene.applyWireframeVisibilityOverride(wireframeVisible);
}
