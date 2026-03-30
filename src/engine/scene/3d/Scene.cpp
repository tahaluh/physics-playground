#include "engine/scene/3d/Scene.h"

#include <algorithm>

Entity &Scene::createEntity(const Entity &entity)
{
    entities.push_back(entity);
    ++revision;
    ++transformRevision;
    return entities.back();
}

void Scene::clearEntities()
{
    entities.clear();
    ++revision;
    ++transformRevision;
}

void Scene::appendEntitiesFrom(const Scene &other)
{
    entities.insert(entities.end(), other.entities.begin(), other.entities.end());
    ++revision;
    ++transformRevision;
}

void Scene::replaceEntitiesFrom(std::initializer_list<const Scene *> sources)
{
    clearEntities();
    for (const Scene *source : sources)
    {
        if (!source)
        {
            continue;
        }

        appendEntitiesFrom(*source);
    }
}

std::vector<Entity> &Scene::getEntities()
{
    return entities;
}

const std::vector<Entity> &Scene::getEntities() const
{
    return entities;
}

uint64_t Scene::getRevision() const
{
    return revision;
}

uint64_t Scene::getTransformRevision() const
{
    return transformRevision;
}

void Scene::touch()
{
    ++revision;
    ++transformRevision;
}

void Scene::touchTransforms()
{
    ++transformRevision;
}

AmbientLight &Scene::getAmbientLight()
{
    return ambientLight;
}

const AmbientLight &Scene::getAmbientLight() const
{
    return ambientLight;
}

bool Scene::copyAmbientLightFromFirstAvailable(std::initializer_list<const Scene *> sources)
{
    for (const Scene *source : sources)
    {
        if (!source)
        {
            continue;
        }

        ambientLight = source->getAmbientLight();
        return true;
    }

    return false;
}

void Scene::applyWireframeVisibilityOverride(bool visible)
{
    for (Entity &entity : entities)
    {
        entity.material.renderWireframe = visible;
    }
    ++revision;
    ++transformRevision;
}

DirectionalLightHandle Scene::createDirectionalLight(const DirectionalLightDesc &desc)
{
    DirectionalLight light;
    light.direction = desc.direction.lengthSquared() > 0.0f ? desc.direction.normalized() : Vector3(0.0f, -1.0f, 0.0f);
    light.color = desc.color;
    light.intensity = desc.intensity;
    light.castShadows = desc.castShadows;
    directionalLights.push_back(light);
    ++revision;
    ++transformRevision;
    return DirectionalLightHandle{directionalLights.size() - 1};
}

void Scene::destroyDirectionalLight(DirectionalLightHandle handle)
{
    if (!handle.isValid() || handle.id >= directionalLights.size())
    {
        return;
    }

    directionalLights[handle.id].enabled = false;
    directionalLights[handle.id].intensity = 0.0f;
    ++revision;
    ++transformRevision;
}

std::vector<DirectionalLight> &Scene::getDirectionalLights()
{
    return directionalLights;
}

const std::vector<DirectionalLight> &Scene::getDirectionalLights() const
{
    return directionalLights;
}

PointLightHandle Scene::createPointLight(const PointLightDesc &desc)
{
    PointLight light;
    light.position = desc.position;
    light.color = desc.color;
    light.intensity = desc.intensity;
    light.range = desc.range > 0.0f ? desc.range : 0.001f;
    light.castShadows = desc.castShadows;
    pointLights.push_back(light);
    ++revision;
    ++transformRevision;
    return PointLightHandle{pointLights.size() - 1};
}

void Scene::destroyPointLight(PointLightHandle handle)
{
    if (!handle.isValid() || handle.id >= pointLights.size())
    {
        return;
    }

    pointLights[handle.id].enabled = false;
    pointLights[handle.id].intensity = 0.0f;
    ++revision;
    ++transformRevision;
}

std::vector<PointLight> &Scene::getPointLights()
{
    return pointLights;
}

const std::vector<PointLight> &Scene::getPointLights() const
{
    return pointLights;
}

SpotLightHandle Scene::createSpotLight(const SpotLightDesc &desc)
{
    SpotLight light;
    light.position = desc.position;
    light.direction = desc.direction.lengthSquared() > 0.0f ? desc.direction.normalized() : Vector3(0.0f, -1.0f, 0.0f);
    light.color = desc.color;
    light.intensity = desc.intensity;
    light.range = desc.range > 0.0f ? desc.range : 0.001f;
    light.innerConeCos = desc.innerConeCos;
    light.outerConeCos = desc.outerConeCos;
    light.castShadows = desc.castShadows;
    if (light.innerConeCos < light.outerConeCos)
    {
        std::swap(light.innerConeCos, light.outerConeCos);
    }
    spotLights.push_back(light);
    ++revision;
    ++transformRevision;
    return SpotLightHandle{spotLights.size() - 1};
}

void Scene::destroySpotLight(SpotLightHandle handle)
{
    if (!handle.isValid() || handle.id >= spotLights.size())
    {
        return;
    }

    spotLights[handle.id].enabled = false;
    spotLights[handle.id].intensity = 0.0f;
    ++revision;
    ++transformRevision;
}

std::vector<SpotLight> &Scene::getSpotLights()
{
    return spotLights;
}

const std::vector<SpotLight> &Scene::getSpotLights() const
{
    return spotLights;
}
