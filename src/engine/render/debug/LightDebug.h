#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "engine/math/Quaternion.h"
#include "engine/math/Vector3.h"
#include "engine/render/3d/mesh/MeshFactory.h"
#include "engine/scene/3d/Scene.h"
#include "engine/scene/3d/DirectionalLight.h"
#include "engine/scene/3d/Entity.h"
#include "engine/scene/3d/PointLight.h"
#include "engine/scene/3d/SpotLight.h"

namespace LightDebug
{
inline Material makeUnlitDebugMaterial(uint32_t color, bool wireframeOnly = false, float wireOpacity = 0.35f)
{
    Material material;
    material.solid.baseColor = 0xFF000000;
    material.solid.emissiveColor = color;
    material.solid.ambientFactor = 0.0f;
    material.solid.diffuseFactor = 0.0f;
    material.solid.unlit = true;
    material.wireframe.baseColor = color;
    material.wireframe.emissiveColor = color;
    material.wireframe.ambientFactor = 0.0f;
    material.wireframe.diffuseFactor = 0.0f;
    material.wireframe.opacity = wireOpacity;
    material.wireframe.unlit = true;
    material.renderSolid = !wireframeOnly;
    material.renderWireframe = wireframeOnly;
    return material;
}

inline Quaternion getRotationFromForward(const Vector3 &direction)
{
    const Vector3 forward = direction.lengthSquared() > 0.0f ? direction.normalized() : Vector3::forward();
    const float planarLength = std::sqrt(forward.x * forward.x + forward.z * forward.z);
    const float pitch = std::atan2(-forward.y, planarLength);
    const float yaw = std::atan2(forward.x, forward.z);
    return Quaternion::fromEulerXYZ(Vector3(pitch, yaw, 0.0f));
}

inline float getConeRadius(float length, float coneCos)
{
    const float clampedCos = Vector3::clamp(coneCos, -0.9999f, 0.9999f);
    const float angle = std::acos(clampedCos);
    return std::tan(angle) * length;
}

inline Entity makePointLightMarker(const PointLight &light)
{
    Entity marker;
    marker.name = "PointLightDebug";
    marker.transform.position = light.position;
    marker.mesh = MeshFactory::makeSphere(0.16f, 10, 16, 0);
    marker.material = makeUnlitDebugMaterial(light.color, false);
    return marker;
}

inline std::vector<Entity> makeSpotLightMarkers(const SpotLight &light)
{
    std::vector<Entity> markers;
    if (!light.enabled)
    {
        return markers;
    }

    const Vector3 direction = light.direction.lengthSquared() > 0.0f ? light.direction.normalized() : Vector3::forward();

    Entity rangeCone;
    rangeCone.name = "SpotLightDebugRange";
    rangeCone.transform.position = light.position;
    rangeCone.transform.rotation = getRotationFromForward(direction);
    rangeCone.mesh = MeshFactory::makeCone(getConeRadius(light.range, light.outerConeCos), light.range, 24, 0);
    rangeCone.material = makeUnlitDebugMaterial(light.color, true, 0.35f);
    markers.push_back(rangeCone);

    return markers;
}

inline Entity makeDirectionalLightMarker(const DirectionalLight &light, const Vector3 &origin, float length = 1.4f)
{
    const Vector3 direction = light.direction.lengthSquared() > 0.0f ? light.direction.normalized() : Vector3::forward();
    Entity marker;
    marker.name = "DirectionalLightDebug";
    marker.transform.position = origin + direction * (length * 0.5f);
    marker.transform.rotation = getRotationFromForward(direction);
    marker.mesh = MeshFactory::makeCone(length * 0.18f, length, 20, 0);
    marker.material = makeUnlitDebugMaterial(light.color, true, 0.65f);
    return marker;
}

inline void appendLightMarkers(const Scene &sourceScene, Scene &targetScene)
{
    for (const PointLight &light : sourceScene.getPointLights())
    {
        if (!light.enabled)
        {
            continue;
        }

        targetScene.createEntity(makePointLightMarker(light));
    }

    for (const SpotLight &light : sourceScene.getSpotLights())
    {
        if (!light.enabled)
        {
            continue;
        }

        const std::vector<Entity> markers = makeSpotLightMarkers(light);
        for (const Entity &marker : markers)
        {
            targetScene.createEntity(marker);
        }
    }
}
}
