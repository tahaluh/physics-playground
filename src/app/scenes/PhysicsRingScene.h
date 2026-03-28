#pragma once

#include <cstddef>
#include <memory>
#include <vector>

class PhysicsBody2D;
class Scene2D;
class Scene3D;

class PhysicsRingScene
{
public:
    ~PhysicsRingScene();

    static std::unique_ptr<PhysicsRingScene> createDefault();

    Scene2D &getPhysicsScene();
    const Scene2D &getPhysicsScene() const;
    Scene3D &getRenderScene();
    const Scene3D &getRenderScene() const;
    PhysicsBody2D *getControlledBody();
    const PhysicsBody2D *getControlledBody() const;

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
};
