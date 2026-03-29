#pragma once

#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"
#include "engine/render/3d/Material3D.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Entity3D.h"

enum class Shape2DIn3DKind
{
    Disc,
    Ring
};

struct Shape2DIn3DDesc
{
    Shape2DIn3DKind kind = Shape2DIn3DKind::Disc;
    const char *name = "Shape2DIn3D";
    Vector2 position = Vector2::zero();
    float rotation = 0.0f;
    float planeZ = 0.0f;
    float radius = 1.0f;
    float innerRadius = 0.5f;
    int segments = 48;
    uint32_t color = 0xFFFFFFFF;
    Material3D material = Material3D{};
};

inline Entity3D makeShape2DIn3D(const Shape2DIn3DDesc &desc)
{
    Entity3D entity;
    entity.name = desc.name;
    entity.transform.position = Vector3(desc.position.x, desc.position.y, desc.planeZ);
    entity.transform.rotation = Vector3(0.0f, 0.0f, desc.rotation);
    entity.material = desc.material;
    entity.material.solid.doubleSidedLighting = true;
    entity.material.wireframe.doubleSidedLighting = true;

    switch (desc.kind)
    {
    case Shape2DIn3DKind::Ring:
        entity.mesh = MeshFactory3D::makeRing(desc.innerRadius, desc.radius, desc.segments, desc.color);
        break;
    case Shape2DIn3DKind::Disc:
    default:
        entity.mesh = MeshFactory3D::makeDisc(desc.radius, desc.segments, desc.color);
        break;
    }

    return entity;
}
