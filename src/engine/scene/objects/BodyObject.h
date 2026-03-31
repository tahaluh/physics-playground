#pragma once

#include <functional>
#include <memory>
#include <string>

#include "engine/math/Transform.h"
#include "engine/physics/CollisionPoints.h"
#include "engine/render/3d/Material.h"

class Collider;
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
    float sleepTime = 0.0f;
    bool sleeping = false;
    bool canSleep = true;
};

struct BodyObjectDesc
{
    std::string name;
    BodyMotionType motionType = BodyMotionType::Static;
    BodyShapeType shapeType = BodyShapeType::Cube;
    bool simulateOnGpu = false;
    Transform transform;
    std::shared_ptr<Collider> collider;
    BodyPhysicsState physics;
    Material material = Material{};
    bool renderEnabled = true;
    int sphereRings = 16;
    int sphereSegments = 24;
};

class BodyObject
{
public:
    using CollisionCallback = std::function<void(BodyObject &, BodyObject &, const CollisionPoints &)>;

    ~BodyObject();

    static std::unique_ptr<BodyObject> create(const BodyObjectDesc &desc);

    bool isValid() const;
    void destroy();

    Scene &getRenderScene();
    const Scene &getRenderScene() const;

    const BodyObjectDesc &getConfig() const;
    const Collider *getCollider() const;
    const Material &getMaterial() const;
    const BodyPhysicsState &getPhysicsState() const;
    void setPhysicsState(const BodyPhysicsState &state);
    bool isSleeping() const;
    void setSleeping(bool sleeping);
    void wakeUp();
    void setCollider(const std::shared_ptr<Collider> &collider);
    void setMaterial(const Material &material);
    void setTransform(const Transform &transform);
    const Transform &getTransform() const;
    void syncRenderScene();
    void notifyCollisionEnter(BodyObject &other, const CollisionPoints &collision);
    void notifyCollisionStay(BodyObject &other, const CollisionPoints &collision);
    void notifyCollisionExit(BodyObject &other, const CollisionPoints &collision);

    CollisionCallback onCollisionEnter;
    CollisionCallback onCollision;
    CollisionCallback onCollisionExit;

private:
    std::unique_ptr<Scene> renderScene;
    std::size_t entityIndex = static_cast<std::size_t>(-1);
    BodyObjectDesc config;
    BodyPhysicsState physicsState;
};
