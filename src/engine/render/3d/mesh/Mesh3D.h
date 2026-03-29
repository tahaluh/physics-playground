#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "engine/math/Vector3.h"

struct MeshEdge3D
{
    int start = 0;
    int end = 0;
};

struct MeshTriangle3D
{
    std::array<int, 3> indices = {0, 0, 0};
    uint32_t color = 0xFFFFFFFF;
};

struct Mesh3D
{
    std::vector<Vector3> vertices;
    std::vector<Vector3> vertexNormals;
    std::vector<MeshEdge3D> edges;
    std::vector<MeshTriangle3D> triangles;
};
