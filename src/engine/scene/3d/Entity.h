#pragma once

#include <string>

#include "engine/math/Transform.h"
#include "engine/render/3d/Material.h"
#include "engine/render/3d/mesh/Mesh.h"

struct Entity
{
    std::string name;
    std::string instancingKey;
    Transform transform;
    Vector3 linearVelocity = Vector3::zero();
    Vector3 angularVelocity = Vector3::zero();
    Mesh mesh;
    Material material;
    bool supportsInstancing = false;
};
