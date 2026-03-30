#pragma once

#include <string>

#include "engine/render/3d/mesh/Mesh.h"

class ObjMeshLoader
{
public:
    static bool loadFromFile(const std::string &path, Mesh &mesh);
};
