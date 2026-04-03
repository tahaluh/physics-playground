#pragma once

#include <functional>
#include <memory>
#include <string>

#include "engine/math/Transform.h"
#include "engine/physics/CollisionPoints.h"
#include "engine/render/3d/Material.h"

class Collider;
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

struct BodyObjectDesc
{
    struct SphereDesc
    {
        int rings = 16;
        int segments = 24;
    } sphere;

    std::string name;
    BodyMotionType motionType = BodyMotionType::Static;
    BodyShapeType shapeType = BodyShapeType::Cube;
    Transform transform;
    Vector3 linearVelocity = Vector3::zero();
    Vector3 angularVelocity = Vector3::zero();
    std::shared_ptr<Collider> collider;
    Material material = Material{};
    bool renderEnabled = true;
};

class BodyObject
{
public:
    using CollisionCallback = std::function<void(BodyObject &, BodyObject &, const CollisionPoints &)>;

    enum class RenderSyncChange
    {
        None = 0,
        Transform = 1 << 0,
        Material = 1 << 1,
    };

    ~BodyObject();

    static std::unique_ptr<BodyObject> create(const BodyObjectDesc &desc);

    bool isValid() const;
    void destroy();

    const BodyObjectDesc &getConfig() const;
    const Collider *getCollider() const;
    bool hasCollider() const;
    const Material &getMaterial() const;
    const Vector3 &getLinearVelocity() const;
    const Vector3 &getAngularVelocity() const;
    bool isSleeping() const;
    void setCollider(const std::shared_ptr<Collider> &collider);
    void setMaterial(const Material &material);
    void setTransform(const Transform &transform);
    void setLinearVelocity(const Vector3 &velocity);
    void setAngularVelocity(const Vector3 &velocity);
    void setVelocities(const Vector3 &linearVelocity, const Vector3 &angularVelocity);
    void setSleeping(bool sleeping);
    void wakeUp();
    const Transform &getTransform() const;
    RenderSyncChange syncRenderScene();
    void notifyCollisionEnter(BodyObject &other, const CollisionPoints &collision);
    void notifyCollisionStay(BodyObject &other, const CollisionPoints &collision);
    void notifyCollisionExit(BodyObject &other, const CollisionPoints &collision);

    CollisionCallback onCollisionEnter;
    CollisionCallback onCollision;
    CollisionCallback onCollisionExit;

private:
    BodyObjectDesc config;
    bool transformDirty = false;
    bool materialDirty = false;
    bool sleeping = false;
};
