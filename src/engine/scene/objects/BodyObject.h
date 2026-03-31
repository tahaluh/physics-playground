#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "engine/math/Transform.h"
#include "engine/physics/CollisionPoints.h"
#include "engine/physics/RigidBody.h"
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
    Plane,
};

using BodyPhysicsState = RigidBody;

struct BodyObjectDesc
{
    std::string name;
    BodyMotionType motionType = BodyMotionType::Static;
    BodyShapeType shapeType = BodyShapeType::Cube;
    bool simulateOnGpu = false;
    Transform transform;
    std::shared_ptr<Collider> collider;
    bool createDefaultCollider = true;
    std::optional<RigidBody> rigidBody;
    float contactFriction = 0.5f;
    float contactRestitution = 0.0f;
    Material material = Material{};
    bool renderEnabled = true;
    int sphereRings = 16;
    int sphereSegments = 24;
};

class BodyObject
{
public:
    using CollisionCallback = std::function<void(BodyObject &, BodyObject &, const CollisionPoints &)>;
    using SleepCallback = std::function<void(BodyObject &)>;

    ~BodyObject();

    static std::unique_ptr<BodyObject> create(const BodyObjectDesc &desc);

    bool isValid() const;
    void destroy();

    Scene &getRenderScene();
    const Scene &getRenderScene() const;

    const BodyObjectDesc &getConfig() const;
    const Collider *getCollider() const;
    bool hasCollider() const;
    const Material &getMaterial() const;
    bool hasRigidBody() const;
    RigidBody *getRigidBody();
    const RigidBody *getRigidBody() const;
    void setRigidBody(const std::optional<RigidBody> &rigidBody);
    void removeRigidBody();
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
    void notifySleep();
    void notifyWakeUp();

    CollisionCallback onCollisionEnter;
    CollisionCallback onCollision;
    CollisionCallback onCollisionExit;
    SleepCallback onSleep;
    SleepCallback onWakeUp;

private:
    std::unique_ptr<Scene> renderScene;
    std::size_t entityIndex = static_cast<std::size_t>(-1);
    BodyObjectDesc config;
    std::optional<RigidBody> rigidBodyState;
};
