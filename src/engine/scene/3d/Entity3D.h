#pragma once

#include <string>

#include "engine/math/Transform3D.h"
#include "engine/render/3d/Material3D.h"
#include "engine/render/3d/mesh/Mesh3D.h"

struct Entity3D
{
    std::string name;
    Transform3D transform;
    Mesh3D mesh;
    Material3D material;
};
