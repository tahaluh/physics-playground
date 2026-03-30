#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "engine/physics/PhysicsSystem.h"
#include "engine/scene/3d/Scene.h"
#include "engine/scene/objects/BodyObject.h"

class RuntimeScene
{
public:
    BodyObject &addBody(const BodyObjectDesc &desc);
    BodyObject &addBody(std::unique_ptr<BodyObject> body);
    void removeBody(const BodyObject &body);
    void clearBodies();
    void step(float dt);
    void setWireframeVisible(bool visible);

    Scene &getRenderScene();
    const Scene &getRenderScene() const;
    std::vector<std::unique_ptr<BodyObject>> &getBodies();
    const std::vector<std::unique_ptr<BodyObject>> &getBodies() const;

private:
    struct SceneEntityRange
    {
        std::size_t start = 0;
        std::size_t count = 0;
    };

    void appendBodyRenderScene(const BodyObject &body);
    void syncBodyRenderScenes();
    void rebuildBodyRenderScene();

    Scene renderScene;
    std::vector<std::unique_ptr<BodyObject>> bodies;
    std::vector<SceneEntityRange> bodyRanges;
    PhysicsSystem physicsSystem;
    bool wireframeVisible = false;
};
