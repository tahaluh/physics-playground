#pragma once

#include "engine/render/3d/mesh/Mesh3D.h"

namespace MeshFactory3D
{
inline Mesh3D makeCube(float size)
{
    const float halfSize = size * 0.5f;

    Mesh3D mesh;
    mesh.vertices = {
        Vector3(-halfSize, -halfSize, -halfSize),
        Vector3(halfSize, -halfSize, -halfSize),
        Vector3(halfSize, halfSize, -halfSize),
        Vector3(-halfSize, halfSize, -halfSize),
        Vector3(-halfSize, -halfSize, halfSize),
        Vector3(halfSize, -halfSize, halfSize),
        Vector3(halfSize, halfSize, halfSize),
        Vector3(-halfSize, halfSize, halfSize),
    };

    mesh.edges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };

    mesh.triangles = {
        {{{0, 1, 2}}, 0xFF3A86FF}, {{{0, 2, 3}}, 0xFF3A86FF},
        {{{4, 5, 6}}, 0xFF4CC9F0}, {{{4, 6, 7}}, 0xFF4CC9F0},
        {{{0, 1, 5}}, 0xFF4361EE}, {{{0, 5, 4}}, 0xFF4361EE},
        {{{2, 3, 7}}, 0xFF4895EF}, {{{2, 7, 6}}, 0xFF4895EF},
        {{{1, 2, 6}}, 0xFF560BAD}, {{{1, 6, 5}}, 0xFF560BAD},
        {{{0, 3, 7}}, 0xFFF72585}, {{{0, 7, 4}}, 0xFFF72585},
    };

    return mesh;
}
}
