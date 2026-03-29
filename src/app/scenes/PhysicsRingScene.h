#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "engine/math/Vector3.h"

class PhysicsBody2D;
class Scene2D;
class Scene3D;

class PhysicsRingScene
{
public:
    ~PhysicsRingScene();

    static std::unique_ptr<PhysicsRingScene> createDefault(bool enableRenderScene = true);

    Scene2D &getPhysicsScene();
    const Scene2D &getPhysicsScene() const;
    bool hasRenderScene() const;
    Scene3D &getRenderScene();
    const Scene3D &getRenderScene() const;
    PhysicsBody2D *getControlledBody();
    const PhysicsBody2D *getControlledBody() const;
    void setWorldOffset(const Vector3 &offset);
    const Vector3 &getWorldOffset() const;

    void syncRenderScene();

private:
    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::unique_ptr<Scene2D> physicsScene;
    std::unique_ptr<Scene3D> renderScene;
    std::size_t controlledBodyIndex = kInvalidIndex;
    std::size_t borderBodyIndex = kInvalidIndex;
    std::size_t borderEntityIndex = kInvalidIndex;
    std::vector<std::size_t> dynamicBodyIndices;
    std::vector<std::size_t> dynamicEntityIndices;
    Vector3 worldOffset = Vector3::zero();
};
