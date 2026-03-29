#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "engine/math/Matrix4.h"
#include "engine/math/Vector3.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Scene3D.h"
#include "engine/scene/3d/DirectionalLight.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/PointLight.h"
#include "engine/scene/3d/SpotLight.h"

namespace LightDebug3D
{
inline Material3D makeUnlitDebugMaterial(uint32_t color, bool wireframeOnly = false, float wireOpacity = 0.35f)
{
    Material3D material;
    material.solid.color = 0xFF000000;
    material.solid.emissiveColor = color;
    material.solid.ambientFactor = 0.0f;
    material.solid.diffuseFactor = 0.0f;
    material.solid.unlit = true;
    material.wireframe.color = color;
    material.wireframe.emissiveColor = color;
    material.wireframe.ambientFactor = 0.0f;
    material.wireframe.diffuseFactor = 0.0f;
    material.wireframe.opacity = wireOpacity;
    material.wireframe.unlit = true;
    material.renderSolid = !wireframeOnly;
    material.renderWireframe = wireframeOnly;
    return material;
}

inline Matrix4 getRotationFromForward(const Vector3 &direction)
{
    const Vector3 from = Vector3::forward();
    const Vector3 to = direction.lengthSquared() > 0.0f ? direction.normalized() : from;
    const float dotValue = Vector3::clamp(from.dot(to), -1.0f, 1.0f);

    if (dotValue > 0.9999f)
    {
        return Matrix4::identity();
    }

    if (dotValue < -0.9999f)
    {
        return Matrix4::axisAngle(Vector3::up(), 3.14159265f);
    }

    const Vector3 axis = from.cross(to).normalized();
    const float angle = std::acos(dotValue);
    return Matrix4::axisAngle(axis, angle);
}

inline float getConeRadius(float length, float coneCos)
{
    const float clampedCos = Vector3::clamp(coneCos, -0.9999f, 0.9999f);
    const float angle = std::acos(clampedCos);
    return std::tan(angle) * length;
}

inline Entity3D makePointLightMarker(const PointLight &light)
{
    Entity3D marker;
    marker.name = "PointLightDebug";
    marker.transform.position = light.position;
    marker.mesh = MeshFactory3D::makeSphere(0.16f, 10, 16, 0);
    marker.material = makeUnlitDebugMaterial(light.color, false);
    return marker;
}

inline std::vector<Entity3D> makeSpotLightMarkers(const SpotLight &light)
{
    std::vector<Entity3D> markers;
    if (!light.enabled)
    {
        return markers;
    }

    const Vector3 direction = light.direction.lengthSquared() > 0.0f ? light.direction.normalized() : Vector3::forward();
    const Matrix4 rotation = getRotationFromForward(direction);

    Entity3D rangeCone;
    rangeCone.name = "SpotLightDebugRange";
    rangeCone.transform.position = light.position;
    rangeCone.transform.setCustomRotationMatrix(rotation);
    rangeCone.mesh = MeshFactory3D::makeCone(getConeRadius(light.range, light.outerConeCos), light.range, 24, 0);
    rangeCone.material = makeUnlitDebugMaterial(light.color, true, 0.35f);
    markers.push_back(rangeCone);

    return markers;
}

inline Entity3D makeDirectionalLightMarker(const DirectionalLight &light, const Vector3 &origin, float length = 1.4f)
{
    const Vector3 direction = light.direction.lengthSquared() > 0.0f ? light.direction.normalized() : Vector3::forward();
    Entity3D marker;
    marker.name = "DirectionalLightDebug";
    marker.transform.position = origin + direction * (length * 0.5f);
    marker.transform.setCustomRotationMatrix(getRotationFromForward(direction));
    marker.mesh = MeshFactory3D::makeCone(length * 0.18f, length, 20, 0);
    marker.material = makeUnlitDebugMaterial(light.color, true, 0.65f);
    return marker;
}

inline void appendLightMarkers(const Scene3D &sourceScene, Scene3D &targetScene)
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

        const std::vector<Entity3D> markers = makeSpotLightMarkers(light);
        for (const Entity3D &marker : markers)
        {
            targetScene.createEntity(marker);
        }
    }
}
}
