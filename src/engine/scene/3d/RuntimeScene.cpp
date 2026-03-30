#include "engine/scene/3d/RuntimeScene.h"

#include <algorithm>
#include <utility>

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
    const auto it = std::find_if(bodies.begin(), bodies.end(), [&body](const std::unique_ptr<BodyObject> &candidate) {
        return candidate.get() == &body;
    });
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
    if (!physicsSystem.step(dt))
    {
        return;
    }

    syncBodyRenderScenes();
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
    range.count = body.getRenderScene().getEntities().size();
    renderScene.appendEntitiesFrom(body.getRenderScene());
    bodyRanges.push_back(range);
}

void RuntimeScene::syncBodyRenderScenes()
{
    std::vector<Entity> &targetEntities = renderScene.getEntities();
    for (std::size_t i = 0; i < bodies.size() && i < bodyRanges.size(); ++i)
    {
        const BodyObject *body = bodies[i].get();
        if (!body)
        {
            continue;
        }

        const SceneEntityRange &range = bodyRanges[i];
        const std::vector<Entity> &sourceEntities = body->getRenderScene().getEntities();
        if (sourceEntities.size() != range.count || range.start + range.count > targetEntities.size())
        {
            continue;
        }

        for (std::size_t entityIndex = 0; entityIndex < range.count; ++entityIndex)
        {
            targetEntities[range.start + entityIndex] = sourceEntities[entityIndex];
        }
    }

    renderScene.touch();
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
}
