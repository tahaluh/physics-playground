#include "engine/scene/3d/Scene3D.h"

Entity3D &Scene3D::createEntity(const Entity3D &entity)
{
    entities.push_back(entity);
    return entities.back();
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
