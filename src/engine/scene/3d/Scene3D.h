#pragma once

#include <vector>

#include "engine/scene/3d/AmbientLight.h"
#include "engine/scene/3d/Entity3D.h"

class Scene3D
{
public:
    Entity3D &createEntity(const Entity3D &entity);
    std::vector<Entity3D> &getEntities();
    const std::vector<Entity3D> &getEntities() const;
    AmbientLight &getAmbientLight();
    const AmbientLight &getAmbientLight() const;

private:
    std::vector<Entity3D> entities;
    AmbientLight ambientLight;
};
