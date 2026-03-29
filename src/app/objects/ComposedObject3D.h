#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "engine/math/Vector3.h"
#include "engine/physics/3d/PhysicsMaterial3D.h"
#include "engine/render/3d/Material3D.h"
#include "engine/render/3d/mesh/Mesh3D.h"

class PhysicsBody3D;
class PhysicsScene3D;
class PhysicsWorld3D;
class Scene3D;

enum class ColliderShape3DType
{
    None,
    Sphere,
};

enum class PhysicsBodyType3D
{
    Static,
    Dynamic,
};

struct ColliderDesc3D
{
    bool enabled = false;
    ColliderShape3DType shapeType = ColliderShape3DType::None;
    float sphereRadius = 0.5f;
};

struct PhysicsBodyDesc3D
{
    bool enabled = false;
    PhysicsBodyType3D bodyType = PhysicsBodyType3D::Static;
    float mass = 1.0f;
    Vector3 startVelocity = Vector3::zero();
    PhysicsMaterial3D material = PhysicsMaterial3D{};
    bool syncRotationFromPhysics = true;
};

struct RenderNodeDesc3D
{
    bool enabled = true;
    Mesh3D mesh;
    Material3D material = Material3D{};
    Vector3 localPosition = Vector3::zero();
    Vector3 localRotation = Vector3::zero();
    Vector3 localScale = Vector3::one();
};

struct SceneBodyNodeDesc3D
{
    std::string name;
    RenderNodeDesc3D render;
    PhysicsBodyDesc3D physics;
    ColliderDesc3D collider;
};

struct ComposedObject3DDesc
{
    std::string name;
    Vector3 worldOffset = Vector3::zero();
    std::vector<SceneBodyNodeDesc3D> nodes;
};

class ComposedObject3D
{
public:
    ~ComposedObject3D();

    static std::unique_ptr<ComposedObject3D> create(const ComposedObject3DDesc &desc);

    bool isValid() const;
    void destroy();

    PhysicsScene3D &getPhysicsScene();
    const PhysicsScene3D &getPhysicsScene() const;
    Scene3D &getRenderScene();
    const Scene3D &getRenderScene() const;

    void setWorldOffset(const Vector3 &offset);
    const Vector3 &getWorldOffset() const;
    void step(PhysicsWorld3D &physicsWorld, float dt);
    void syncRenderScene();

private:
    struct NodeBinding
    {
        std::string name;
        Vector3 localPosition = Vector3::zero();
        Vector3 localRotation = Vector3::zero();
        Vector3 localScale = Vector3::one();
        bool renderEnabled = false;
        bool physicsEnabled = false;
        bool syncRotationFromPhysics = true;
        std::size_t bodyIndex = kInvalidIndex;
        std::size_t entityIndex = kInvalidIndex;
    };

    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::unique_ptr<PhysicsScene3D> physicsScene;
    std::unique_ptr<Scene3D> renderScene;
    ComposedObject3DDesc config;
    Vector3 worldOffset = Vector3::zero();
    std::vector<NodeBinding> bindings;
};
