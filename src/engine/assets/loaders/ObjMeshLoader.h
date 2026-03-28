#pragma once

#include <string>

#include "engine/render/3d/mesh/Mesh3D.h"

class ObjMeshLoader
{
public:
    static bool loadFromFile(const std::string &path, Mesh3D &mesh);
};
