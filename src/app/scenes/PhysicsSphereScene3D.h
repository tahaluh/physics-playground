#pragma once

#include <cstddef>
#include <memory>

#include "engine/math/Vector3.h"

class PhysicsBody3D;
class PhysicsScene3D;
class Scene3D;

class PhysicsSphereScene3D
{
public:
    ~PhysicsSphereScene3D();

    static std::unique_ptr<PhysicsSphereScene3D> createDefault();

    PhysicsScene3D &getPhysicsScene();
    const PhysicsScene3D &getPhysicsScene() const;
    Scene3D &getRenderScene();
    const Scene3D &getRenderScene() const;
    PhysicsBody3D *getBallBody();
    const PhysicsBody3D *getBallBody() const;
    void setWorldOffset(const Vector3 &offset);
    const Vector3 &getWorldOffset() const;

    void syncRenderScene();

private:
    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::unique_ptr<PhysicsScene3D> physicsScene;
    std::unique_ptr<Scene3D> renderScene;
    std::size_t borderBodyIndex = kInvalidIndex;
    std::size_t ballBodyIndex = kInvalidIndex;
    std::size_t borderEntityIndex = kInvalidIndex;
    std::size_t ballEntityIndex = kInvalidIndex;
    Vector3 worldOffset = Vector3::zero();
};
