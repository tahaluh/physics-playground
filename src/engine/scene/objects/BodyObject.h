#pragma once

#include <memory>
#include <string>

#include "engine/math/Transform.h"
#include "engine/render/3d/Material.h"

class Scene;

enum class BodyMotionType
{
    Static,
    Dynamic,
    Kinematic,
};

enum class BodyShapeType
{
    Cube,
    Sphere,
};

struct BodyPhysicsState
{
    float mass = 1.0f;
    Vector3 linearVelocity = Vector3::zero();
    Vector3 angularVelocity = Vector3::zero();
};

struct BodyObjectDesc
{
    std::string name;
    BodyMotionType motionType = BodyMotionType::Static;
    BodyShapeType shapeType = BodyShapeType::Cube;
    Transform transform;
    BodyPhysicsState physics;
    Material material = Material{};
    bool renderEnabled = true;
    int sphereRings = 16;
    int sphereSegments = 24;
};

class BodyObject
{
public:
    ~BodyObject();

    static std::unique_ptr<BodyObject> create(const BodyObjectDesc &desc);

    bool isValid() const;
    void destroy();

    Scene &getRenderScene();
    const Scene &getRenderScene() const;

    const BodyObjectDesc &getConfig() const;
    const BodyPhysicsState &getPhysicsState() const;
    void setPhysicsState(const BodyPhysicsState &state);
    void setTransform(const Transform &transform);
    const Transform &getTransform() const;
    void syncRenderScene();

private:
    std::unique_ptr<Scene> renderScene;
    std::size_t entityIndex = static_cast<std::size_t>(-1);
    BodyObjectDesc config;
    BodyPhysicsState physicsState;
};
