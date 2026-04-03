#include "engine/scene/objects/BodyObject.h"

#include <memory>

#include "engine/physics/BoxCollider.h"
#include "engine/physics/PlaneCollider.h"
#include "engine/render/3d/mesh/MeshFactory.h"

namespace
{
    std::shared_ptr<Collider> makeDefaultCollider(const BodyObjectDesc &desc)
    {
        switch (desc.shapeType)
        {
        case BodyShapeType::Cube:
            return std::make_shared<BoxCollider>(Vector3::zero(), Vector3::one() * 0.5f);
        case BodyShapeType::Plane:
            return std::make_shared<PlaneCollider>(Vector3::zero(), Vector2(0.5f, 0.5f));
        case BodyShapeType::Sphere:
        default:
            return std::make_shared<BoxCollider>(Vector3::zero(), Vector3::one() * 0.5f);
        }
    }

    Mesh makeBodyMesh(const BodyObjectDesc &desc)
    {
        switch (desc.shapeType)
        {
        case BodyShapeType::Plane:
            return MeshFactory::makeQuadXZ(1.0f);
        case BodyShapeType::Sphere:
            return MeshFactory::makeSphere(0.5f, desc.sphere.rings, desc.sphere.segments, 0);
        case BodyShapeType::Cube:
        default:
            return MeshFactory::makeCube(1.0f);
        }
    }
}

BodyObject::~BodyObject() = default;

std::unique_ptr<BodyObject> BodyObject::create(const BodyObjectDesc &desc)
{
    auto object = std::make_unique<BodyObject>();
    object->config = desc;
    object->config.collider = desc.collider ? desc.collider->clone() : makeDefaultCollider(desc);
    object->transformDirty = false;
    object->materialDirty = false;

    return object;
}

bool BodyObject::isValid() const
{
    return true;
}

void BodyObject::destroy()
{
}

const BodyObjectDesc &BodyObject::getConfig() const
{
    return config;
}

const Collider *BodyObject::getCollider() const
{
    return config.collider.get();
}

bool BodyObject::hasCollider() const
{
    return static_cast<bool>(config.collider);
}

bool BodyObject::isSleeping() const
{
    return sleeping;
}

const Material &BodyObject::getMaterial() const
{
    return config.material;
}

const Vector3 &BodyObject::getLinearVelocity() const
{
    return config.linearVelocity;
}

const Vector3 &BodyObject::getAngularVelocity() const
{
    return config.angularVelocity;
}

void BodyObject::setCollider(const std::shared_ptr<Collider> &collider)
{
    config.collider = collider ? collider->clone() : nullptr;
}

void BodyObject::setMaterial(const Material &material)
{
    config.material = material;
    materialDirty = true;
}

void BodyObject::setTransform(const Transform &transform)
{
    config.transform = transform;
    transformDirty = true;
    sleeping = false;
}

void BodyObject::setLinearVelocity(const Vector3 &velocity)
{
    config.linearVelocity = velocity;
    sleeping = velocity.lengthSquared() <= 0.0f ? sleeping : false;
}

void BodyObject::setAngularVelocity(const Vector3 &velocity)
{
    config.angularVelocity = velocity;
    sleeping = velocity.lengthSquared() <= 0.0f ? sleeping : false;
}

void BodyObject::setVelocities(const Vector3 &linearVelocity, const Vector3 &angularVelocity)
{
    config.linearVelocity = linearVelocity;
    config.angularVelocity = angularVelocity;
    if (linearVelocity.lengthSquared() > 0.0f || angularVelocity.lengthSquared() > 0.0f)
    {
        sleeping = false;
    }
}

void BodyObject::setSleeping(bool value)
{
    sleeping = value;
}

void BodyObject::wakeUp()
{
    sleeping = false;
}

const Transform &BodyObject::getTransform() const
{
    return config.transform;
}

void BodyObject::notifyCollisionEnter(BodyObject &other, const CollisionPoints &collision)
{
    if (onCollisionEnter)
    {
        onCollisionEnter(*this, other, collision);
    }
}

void BodyObject::notifyCollisionStay(BodyObject &other, const CollisionPoints &collision)
{
    if (onCollision)
    {
        onCollision(*this, other, collision);
    }
}

void BodyObject::notifyCollisionExit(BodyObject &other, const CollisionPoints &collision)
{
    if (onCollisionExit)
    {
        onCollisionExit(*this, other, collision);
    }
}

BodyObject::RenderSyncChange BodyObject::syncRenderScene()
{
    bool transformChanged = false;
    bool materialChanged = false;
    if (transformDirty)
    {
        transformChanged = true;
    }
    if (materialDirty)
    {
        materialChanged = true;
    }

    transformDirty = false;
    materialDirty = false;

    if (transformChanged && materialChanged)
    {
        return static_cast<RenderSyncChange>(static_cast<int>(RenderSyncChange::Transform) | static_cast<int>(RenderSyncChange::Material));
    }
    if (transformChanged)
    {
        return RenderSyncChange::Transform;
    }
    if (materialChanged)
    {
        return RenderSyncChange::Material;
    }
    return RenderSyncChange::None;
}
