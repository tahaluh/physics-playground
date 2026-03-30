#pragma once

#include <cstddef>
#include <initializer_list>
#include <vector>

#include "engine/scene/3d/AmbientLight.h"
#include "engine/scene/3d/DirectionalLight.h"
#include "engine/scene/3d/Entity.h"
#include "engine/scene/3d/PointLight.h"
#include "engine/scene/3d/SpotLight.h"

class Scene
{
public:
    Entity &createEntity(const Entity &entity);
    void clearEntities();
    void appendEntitiesFrom(const Scene &other);
    void replaceEntitiesFrom(std::initializer_list<const Scene *> sources);
    std::vector<Entity> &getEntities();
    const std::vector<Entity> &getEntities() const;
    uint64_t getRevision() const;
    void touch();
    AmbientLight &getAmbientLight();
    const AmbientLight &getAmbientLight() const;
    bool copyAmbientLightFromFirstAvailable(std::initializer_list<const Scene *> sources);
    void applyWireframeVisibilityOverride(bool visible);
    DirectionalLightHandle createDirectionalLight(const DirectionalLightDesc &desc);
    void destroyDirectionalLight(DirectionalLightHandle handle);
    std::vector<DirectionalLight> &getDirectionalLights();
    const std::vector<DirectionalLight> &getDirectionalLights() const;
    PointLightHandle createPointLight(const PointLightDesc &desc);
    void destroyPointLight(PointLightHandle handle);
    std::vector<PointLight> &getPointLights();
    const std::vector<PointLight> &getPointLights() const;
    SpotLightHandle createSpotLight(const SpotLightDesc &desc);
    void destroySpotLight(SpotLightHandle handle);
    std::vector<SpotLight> &getSpotLights();
    const std::vector<SpotLight> &getSpotLights() const;

private:
    std::vector<Entity> entities;
    uint64_t revision = 1;
    AmbientLight ambientLight;
    std::vector<DirectionalLight> directionalLights;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
};
