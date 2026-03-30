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
    Mesh mesh;
    Material material;
    bool supportsInstancing = false;
};
