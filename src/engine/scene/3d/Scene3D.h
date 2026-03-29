#pragma once

#include <cstddef>
#include <vector>

#include "engine/scene/3d/AmbientLight.h"
#include "engine/scene/3d/DirectionalLight.h"
#include "engine/scene/3d/Entity3D.h"

class Scene3D
{
public:
    Entity3D &createEntity(const Entity3D &entity);
    void clearEntities();
    void appendEntitiesFrom(const Scene3D &other);
    std::vector<Entity3D> &getEntities();
    const std::vector<Entity3D> &getEntities() const;
    AmbientLight &getAmbientLight();
    const AmbientLight &getAmbientLight() const;
    DirectionalLightHandle createDirectionalLight(const DirectionalLightDesc &desc);
    void destroyDirectionalLight(DirectionalLightHandle handle);
    std::vector<DirectionalLight> &getDirectionalLights();
    const std::vector<DirectionalLight> &getDirectionalLights() const;

private:
    std::vector<Entity3D> entities;
    AmbientLight ambientLight;
    std::vector<DirectionalLight> directionalLights;
};
