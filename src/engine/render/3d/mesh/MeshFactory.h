#pragma once

#include <cmath>
#include <cstdint>
#include <unordered_map>

#include "engine/render/3d/mesh/Mesh.h"

namespace MeshFactory
{
inline Mesh makeDisc(float radius, int segments = 48, uint32_t color = 0xFFFFFFFF)
{
    Mesh mesh;
    mesh.vertices.reserve(static_cast<size_t>(segments) + 1);
    mesh.vertexNormals.reserve(static_cast<size_t>(segments) + 1);
    mesh.vertices.push_back(Vector3(0.0f, 0.0f, 0.0f));
    mesh.vertexNormals.push_back(Vector3(0.0f, 0.0f, 1.0f));

    const float step = 2.0f * 3.14159265f / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
        const float angle = step * static_cast<float>(i);
        mesh.vertices.push_back(Vector3(std::cos(angle) * radius, std::sin(angle) * radius, 0.0f));
        mesh.vertexNormals.push_back(Vector3(0.0f, 0.0f, 1.0f));
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

inline Mesh makeRing(float innerRadius, float outerRadius, int segments = 64, uint32_t color = 0xFFFFFFFF)
{
    Mesh mesh;
    mesh.vertices.reserve(static_cast<size_t>(segments) * 2);
    mesh.vertexNormals.reserve(static_cast<size_t>(segments) * 2);

    const float step = 2.0f * 3.14159265f / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
        const float angle = step * static_cast<float>(i);
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        mesh.vertices.push_back(Vector3(c * outerRadius, s * outerRadius, 0.0f));
        mesh.vertices.push_back(Vector3(c * innerRadius, s * innerRadius, 0.0f));
        mesh.vertexNormals.push_back(Vector3(0.0f, 0.0f, 1.0f));
        mesh.vertexNormals.push_back(Vector3(0.0f, 0.0f, 1.0f));
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

inline Mesh makeDoubleSidedRing(
    float innerRadius,
    float outerRadius,
    float thickness,
    int segments = 64,
    uint32_t color = 0xFFFFFFFF)
{
    Mesh mesh;
    const float halfThickness = thickness * 0.5f;
    mesh.vertices.reserve(static_cast<size_t>(segments) * 4);

    const float step = 2.0f * 3.14159265f / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
        const float angle = step * static_cast<float>(i);
        const float c = std::cos(angle);
        const float s = std::sin(angle);

        mesh.vertices.push_back(Vector3(c * outerRadius, s * outerRadius, halfThickness));
        mesh.vertices.push_back(Vector3(c * innerRadius, s * innerRadius, halfThickness));
        mesh.vertices.push_back(Vector3(c * outerRadius, s * outerRadius, -halfThickness));
        mesh.vertices.push_back(Vector3(c * innerRadius, s * innerRadius, -halfThickness));
    }

    for (int i = 0; i < segments; ++i)
    {
        const int next = (i + 1) % segments;

        const int outerFrontCurrent = i * 4;
        const int innerFrontCurrent = outerFrontCurrent + 1;
        const int outerBackCurrent = outerFrontCurrent + 2;
        const int innerBackCurrent = outerFrontCurrent + 3;

        const int outerFrontNext = next * 4;
        const int innerFrontNext = outerFrontNext + 1;
        const int outerBackNext = outerFrontNext + 2;
        const int innerBackNext = outerFrontNext + 3;

        mesh.edges.push_back({outerFrontCurrent, outerFrontNext});
        mesh.edges.push_back({innerFrontCurrent, innerFrontNext});
        mesh.edges.push_back({outerBackCurrent, outerBackNext});
        mesh.edges.push_back({innerBackCurrent, innerBackNext});
        mesh.edges.push_back({outerFrontCurrent, outerBackCurrent});
        mesh.edges.push_back({innerFrontCurrent, innerBackCurrent});

        mesh.triangles.push_back({{outerFrontCurrent, innerFrontCurrent, outerFrontNext}, color});
        mesh.triangles.push_back({{outerFrontNext, innerFrontCurrent, innerFrontNext}, color});

        mesh.triangles.push_back({{outerBackCurrent, outerBackNext, innerBackCurrent}, color});
        mesh.triangles.push_back({{outerBackNext, innerBackNext, innerBackCurrent}, color});

        mesh.triangles.push_back({{outerFrontCurrent, outerBackCurrent, outerFrontNext}, color});
        mesh.triangles.push_back({{outerFrontNext, outerBackCurrent, outerBackNext}, color});

        mesh.triangles.push_back({{innerFrontCurrent, innerFrontNext, innerBackCurrent}, color});
        mesh.triangles.push_back({{innerFrontNext, innerBackNext, innerBackCurrent}, color});
    }

    return mesh;
}

inline Mesh makeCube(float size, uint32_t color = 0)
{
    const float halfSize = size * 0.5f;

    Mesh mesh;
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
        {{{0, 2, 1}}, color}, {{{0, 3, 2}}, color},
        {{{4, 5, 6}}, color}, {{{4, 6, 7}}, color},
        {{{0, 1, 5}}, color}, {{{0, 5, 4}}, color},
        {{{2, 3, 7}}, color}, {{{2, 7, 6}}, color},
        {{{1, 2, 6}}, color}, {{{1, 6, 5}}, color},
        {{{0, 4, 7}}, color}, {{{0, 7, 3}}, color},
    };

    return mesh;
}

inline Mesh makeQuadXZ(float size, uint32_t color = 0xFFFFFFFF)
{
    const float halfSize = size * 0.5f;

    Mesh mesh;
    mesh.vertices = {
        Vector3(-halfSize, 0.0f, -halfSize),
        Vector3(halfSize, 0.0f, -halfSize),
        Vector3(halfSize, 0.0f, halfSize),
        Vector3(-halfSize, 0.0f, halfSize),
    };

    mesh.edges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
    };

    mesh.vertexNormals = {
        Vector3::up(),
        Vector3::up(),
        Vector3::up(),
        Vector3::up(),
    };

    mesh.triangles = {
        {{{0, 1, 2}}, color},
        {{{0, 2, 3}}, color},
    };

    return mesh;
}

inline Mesh makeCone(float radius, float height, int segments = 24, uint32_t color = 0xFFFFFFFF)
{
    Mesh mesh;
    mesh.vertices.reserve(static_cast<size_t>(segments) + 2);

    const int tipIndex = 0;
    const int baseCenterIndex = 1;
    mesh.vertices.push_back(Vector3(0.0f, 0.0f, 0.0f));
    mesh.vertices.push_back(Vector3(0.0f, 0.0f, height));

    const float step = 2.0f * 3.14159265f / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
        const float angle = step * static_cast<float>(i);
        mesh.vertices.push_back(Vector3(std::cos(angle) * radius, std::sin(angle) * radius, height));
    }

    for (int i = 0; i < segments; ++i)
    {
        const int current = i + 2;
        const int next = ((i + 1) % segments) + 2;

        mesh.edges.push_back({tipIndex, current});
        mesh.edges.push_back({current, next});
        mesh.triangles.push_back({{tipIndex, current, next}, color});
        mesh.triangles.push_back({{baseCenterIndex, next, current}, color});
    }

    return mesh;
}

inline Mesh makeSphere(float radius, int latitudeSegments = 12, int longitudeSegments = 24, uint32_t color = 0xFFFFFFFF)
{
    Mesh mesh;
    mesh.vertices.reserve(static_cast<size_t>(latitudeSegments + 1) * static_cast<size_t>(longitudeSegments + 1));
    mesh.vertexNormals.reserve(static_cast<size_t>(latitudeSegments + 1) * static_cast<size_t>(longitudeSegments + 1));

    const float pi = 3.14159265f;
    for (int lat = 0; lat <= latitudeSegments; ++lat)
    {
        const float v = static_cast<float>(lat) / static_cast<float>(latitudeSegments);
        const float phi = v * pi;
        const float y = std::cos(phi) * radius;
        const float ringRadius = std::sin(phi) * radius;

        for (int lon = 0; lon <= longitudeSegments; ++lon)
        {
            const float u = static_cast<float>(lon) / static_cast<float>(longitudeSegments);
            const float theta = u * 2.0f * pi;
            mesh.vertices.push_back(Vector3(
                std::cos(theta) * ringRadius,
                y,
                std::sin(theta) * ringRadius));
            mesh.vertexNormals.push_back(mesh.vertices.back().normalized());
        }
    }

    const int stride = longitudeSegments + 1;
    for (int lat = 0; lat < latitudeSegments; ++lat)
    {
        for (int lon = 0; lon < longitudeSegments; ++lon)
        {
            const int current = lat * stride + lon;
            const int next = current + stride;
            const int currentNext = current + 1;
            const int nextNext = next + 1;

            mesh.edges.push_back({current, currentNext});
            mesh.edges.push_back({current, next});
            mesh.triangles.push_back({{current, currentNext, next}, color});
            mesh.triangles.push_back({{currentNext, nextNext, next}, color});
        }
    }

    return mesh;
}

inline Mesh makeIcoSphere(float radius, int subdivisions = 2, uint32_t color = 0xFFFFFFFF)
{
    Mesh mesh;

    const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;
    mesh.vertices = {
        Vector3(-1.0f, t, 0.0f), Vector3(1.0f, t, 0.0f), Vector3(-1.0f, -t, 0.0f), Vector3(1.0f, -t, 0.0f),
        Vector3(0.0f, -1.0f, t), Vector3(0.0f, 1.0f, t), Vector3(0.0f, -1.0f, -t), Vector3(0.0f, 1.0f, -t),
        Vector3(t, 0.0f, -1.0f), Vector3(t, 0.0f, 1.0f), Vector3(-t, 0.0f, -1.0f), Vector3(-t, 0.0f, 1.0f),
    };

    for (Vector3 &vertex : mesh.vertices)
    {
        vertex = vertex.normalized() * radius;
    }

    std::vector<MeshTriangle> triangles = {
        {{0, 11, 5}, color}, {{0, 5, 1}, color}, {{0, 1, 7}, color}, {{0, 7, 10}, color}, {{0, 10, 11}, color},
        {{1, 5, 9}, color}, {{5, 11, 4}, color}, {{11, 10, 2}, color}, {{10, 7, 6}, color}, {{7, 1, 8}, color},
        {{3, 9, 4}, color}, {{3, 4, 2}, color}, {{3, 2, 6}, color}, {{3, 6, 8}, color}, {{3, 8, 9}, color},
        {{4, 9, 5}, color}, {{2, 4, 11}, color}, {{6, 2, 10}, color}, {{8, 6, 7}, color}, {{9, 8, 1}, color},
    };

    const auto midpointKey = [](int a, int b) -> uint64_t
    {
        const uint32_t low = static_cast<uint32_t>(std::min(a, b));
        const uint32_t high = static_cast<uint32_t>(std::max(a, b));
        return (static_cast<uint64_t>(low) << 32) | static_cast<uint64_t>(high);
    };

    for (int subdivision = 0; subdivision < subdivisions; ++subdivision)
    {
        std::unordered_map<uint64_t, int> midpointCache;
        std::vector<MeshTriangle> subdividedTriangles;
        subdividedTriangles.reserve(triangles.size() * 4);

        const auto getMidpointIndex = [&](int a, int b) -> int
        {
            const uint64_t key = midpointKey(a, b);
            const auto found = midpointCache.find(key);
            if (found != midpointCache.end())
            {
                return found->second;
            }

            const Vector3 midpoint = (mesh.vertices[a] + mesh.vertices[b]) * 0.5f;
            const int index = static_cast<int>(mesh.vertices.size());
            mesh.vertices.push_back(midpoint.normalized() * radius);
            midpointCache.emplace(key, index);
            return index;
        };

        for (const MeshTriangle &triangle : triangles)
        {
            const int a = triangle.indices[0];
            const int b = triangle.indices[1];
            const int c = triangle.indices[2];
            const int ab = getMidpointIndex(a, b);
            const int bc = getMidpointIndex(b, c);
            const int ca = getMidpointIndex(c, a);

            subdividedTriangles.push_back({{a, ab, ca}, triangle.color});
            subdividedTriangles.push_back({{b, bc, ab}, triangle.color});
            subdividedTriangles.push_back({{c, ca, bc}, triangle.color});
            subdividedTriangles.push_back({{ab, bc, ca}, triangle.color});
        }

        triangles = std::move(subdividedTriangles);
    }

    mesh.vertexNormals.reserve(mesh.vertices.size());
    for (const Vector3 &vertex : mesh.vertices)
    {
        mesh.vertexNormals.push_back(vertex.normalized());
    }

    mesh.triangles = triangles;

    std::unordered_map<uint64_t, bool> uniqueEdges;
    for (const MeshTriangle &triangle : mesh.triangles)
    {
        for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
        {
            const int start = triangle.indices[edgeIndex];
            const int end = triangle.indices[(edgeIndex + 1) % 3];
            const uint64_t key = midpointKey(start, end);
            if (uniqueEdges.emplace(key, true).second)
            {
                mesh.edges.push_back({start, end});
            }
        }
    }

    return mesh;
}
}
