#pragma once

#include <cmath>

#include "engine/render/3d/mesh/Mesh3D.h"

namespace MeshFactory3D
{
inline Mesh3D makeDisc(float radius, int segments = 48, uint32_t color = 0xFFFFFFFF)
{
    Mesh3D mesh;
    mesh.vertices.reserve(static_cast<size_t>(segments) + 1);
    mesh.vertices.push_back(Vector3(0.0f, 0.0f, 0.0f));

    const float step = 2.0f * 3.14159265f / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
        const float angle = step * static_cast<float>(i);
        mesh.vertices.push_back(Vector3(std::cos(angle) * radius, std::sin(angle) * radius, 0.0f));
    }

    for (int i = 0; i < segments; ++i)
    {
        const int current = i + 1;
        const int next = ((i + 1) % segments) + 1;
        mesh.edges.push_back({current, next});
        mesh.triangles.push_back({{0, current, next}, color});
    }

    return mesh;
}

inline Mesh3D makeRing(float innerRadius, float outerRadius, int segments = 64, uint32_t color = 0xFFFFFFFF)
{
    Mesh3D mesh;
    mesh.vertices.reserve(static_cast<size_t>(segments) * 2);

    const float step = 2.0f * 3.14159265f / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
        const float angle = step * static_cast<float>(i);
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        mesh.vertices.push_back(Vector3(c * outerRadius, s * outerRadius, 0.0f));
        mesh.vertices.push_back(Vector3(c * innerRadius, s * innerRadius, 0.0f));
    }

    for (int i = 0; i < segments; ++i)
    {
        const int next = (i + 1) % segments;
        const int outerCurrent = i * 2;
        const int innerCurrent = outerCurrent + 1;
        const int outerNext = next * 2;
        const int innerNext = outerNext + 1;

        mesh.edges.push_back({outerCurrent, outerNext});
        mesh.edges.push_back({innerCurrent, innerNext});
        mesh.triangles.push_back({{outerCurrent, innerCurrent, outerNext}, color});
        mesh.triangles.push_back({{outerNext, innerCurrent, innerNext}, color});
    }

    return mesh;
}

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
