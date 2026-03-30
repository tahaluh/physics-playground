#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "engine/math/Vector3.h"

struct MeshEdge
{
    int start = 0;
    int end = 0;
};

struct MeshTriangle
{
    std::array<int, 3> indices = {0, 0, 0};
    uint32_t color = 0xFFFFFFFF;
};

struct Mesh
{
    std::vector<Vector3> vertices;
    std::vector<Vector3> vertexNormals;
    std::vector<MeshEdge> edges;
    std::vector<MeshTriangle> triangles;
};
