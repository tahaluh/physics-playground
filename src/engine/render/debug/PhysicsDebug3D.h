#pragma once

#include "engine/physics/3d/shapes/BoxCollider3D.h"
#include "engine/physics/3d/PhysicsBody3D.h"
#include "engine/physics/3d/shapes/SphereShape3D.h"
#include "engine/render/debug/LightDebug3D.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/PhysicsScene3D.h"
#include "engine/scene/3d/Scene3D.h"

namespace PhysicsDebug3D
{
inline Entity3D makeBodyCenterMarker(const PhysicsBody3D &body, const Vector3 &worldOffset)
{
    Entity3D marker;
    marker.name = body.isStatic() ? "StaticBodyCenterDebug" : "DynamicBodyCenterDebug";
    marker.transform.position = worldOffset + body.getPosition();
    marker.mesh = MeshFactory3D::makeSphere(body.isStatic() ? 0.08f : 0.12f, 8, 12, 0);
    marker.material = LightDebug3D::makeUnlitDebugMaterial(body.isStatic() ? 0xFFFFB347 : 0xFF59E3FF, false, 0.4f);
    return marker;
}

inline void appendPhysicsSceneMarkers(const PhysicsScene3D &sourceScene, Scene3D &targetScene, const Vector3 &worldOffset = Vector3::zero())
{
    for (const auto &bodyPtr : sourceScene.getBodies())
    {
        if (!bodyPtr)
        {
            continue;
        }

        const PhysicsBody3D &body = *bodyPtr;
        targetScene.createEntity(makeBodyCenterMarker(body, worldOffset));

        if (const auto *sphere = dynamic_cast<const SphereShape3D *>(body.getShape()))
        {
            Entity3D collider;
            collider.name = body.isStatic() ? "StaticSphereColliderDebug" : "DynamicSphereColliderDebug";
            collider.transform.position = worldOffset + body.getPosition();
            collider.mesh = MeshFactory3D::makeSphere(sphere->getRadius(), 10, 16, 0);
            collider.material = LightDebug3D::makeUnlitDebugMaterial(body.isStatic() ? 0xFFFFA84D : 0xFF00D1FF, true, 0.85f);
            targetScene.createEntity(collider);
        }

        if (const auto *box = dynamic_cast<const BoxCollider3D *>(body.getShape()))
        {
            const Vector3 halfExtents = box->getHalfExtents();
            Entity3D collider;
            collider.name = body.isStatic() ? "StaticBoxColliderDebug" : "DynamicBoxColliderDebug";
            collider.transform.position = worldOffset + body.getPosition();
            collider.transform.rotation = Vector3::zero();
            collider.transform.setCustomRotationMatrix(body.getRotationMatrix());
            collider.mesh = MeshFactory3D::makeCube(1.0f);
            collider.transform.scale = halfExtents * 2.0f;
            collider.material = LightDebug3D::makeUnlitDebugMaterial(body.isStatic() ? 0xFFFFA84D : 0xFF00FF9F, true, 0.85f);
            targetScene.createEntity(collider);
        }
    }
}
}
