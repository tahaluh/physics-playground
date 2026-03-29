#include "engine/scene/3d/Scene3D.h"

#include <algorithm>

Entity3D &Scene3D::createEntity(const Entity3D &entity)
{
    entities.push_back(entity);
    return entities.back();
}

void Scene3D::clearEntities()
{
    entities.clear();
}

void Scene3D::appendEntitiesFrom(const Scene3D &other)
{
    entities.insert(entities.end(), other.entities.begin(), other.entities.end());
}

std::vector<Entity3D> &Scene3D::getEntities()
{
    return entities;
}

const std::vector<Entity3D> &Scene3D::getEntities() const
{
    return entities;
}

AmbientLight &Scene3D::getAmbientLight()
{
    return ambientLight;
}

const AmbientLight &Scene3D::getAmbientLight() const
{
    return ambientLight;
}

DirectionalLightHandle Scene3D::createDirectionalLight(const DirectionalLightDesc &desc)
{
    DirectionalLight light;
    light.direction = desc.direction.lengthSquared() > 0.0f ? desc.direction.normalized() : Vector3(0.0f, -1.0f, 0.0f);
    light.color = desc.color;
    light.intensity = desc.intensity;
    directionalLights.push_back(light);
    return DirectionalLightHandle{directionalLights.size() - 1};
}

void Scene3D::destroyDirectionalLight(DirectionalLightHandle handle)
{
    if (!handle.isValid() || handle.id >= directionalLights.size())
    {
        return;
    }

    directionalLights[handle.id].enabled = false;
    directionalLights[handle.id].intensity = 0.0f;
}

std::vector<DirectionalLight> &Scene3D::getDirectionalLights()
{
    return directionalLights;
}

const std::vector<DirectionalLight> &Scene3D::getDirectionalLights() const
{
    return directionalLights;
}

PointLightHandle Scene3D::createPointLight(const PointLightDesc &desc)
{
    PointLight light;
    light.position = desc.position;
    light.color = desc.color;
    light.intensity = desc.intensity;
    light.range = desc.range > 0.0f ? desc.range : 0.001f;
    pointLights.push_back(light);
    return PointLightHandle{pointLights.size() - 1};
}

void Scene3D::destroyPointLight(PointLightHandle handle)
{
    if (!handle.isValid() || handle.id >= pointLights.size())
    {
        return;
    }

    pointLights[handle.id].enabled = false;
    pointLights[handle.id].intensity = 0.0f;
}

std::vector<PointLight> &Scene3D::getPointLights()
{
    return pointLights;
}

const std::vector<PointLight> &Scene3D::getPointLights() const
{
    return pointLights;
}

SpotLightHandle Scene3D::createSpotLight(const SpotLightDesc &desc)
{
    SpotLight light;
    light.position = desc.position;
    light.direction = desc.direction.lengthSquared() > 0.0f ? desc.direction.normalized() : Vector3(0.0f, -1.0f, 0.0f);
    light.color = desc.color;
    light.intensity = desc.intensity;
    light.range = desc.range > 0.0f ? desc.range : 0.001f;
    light.innerConeCos = desc.innerConeCos;
    light.outerConeCos = desc.outerConeCos;
    if (light.innerConeCos < light.outerConeCos)
    {
        std::swap(light.innerConeCos, light.outerConeCos);
    }
    spotLights.push_back(light);
    return SpotLightHandle{spotLights.size() - 1};
}

void Scene3D::destroySpotLight(SpotLightHandle handle)
{
    if (!handle.isValid() || handle.id >= spotLights.size())
    {
        return;
    }

    spotLights[handle.id].enabled = false;
    spotLights[handle.id].intensity = 0.0f;
}

std::vector<SpotLight> &Scene3D::getSpotLights()
{
    return spotLights;
}

const std::vector<SpotLight> &Scene3D::getSpotLights() const
{
    return spotLights;
}
