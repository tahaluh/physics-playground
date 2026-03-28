#include "engine/render/3d/SolidRenderer3D.h"

#include <algorithm>

#include "engine/math/Vector2.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/render/3d/CubeMesh3D.h"
#include "engine/render/3d/ProjectedVertex3D.h"
#include "engine/render/3d/RenderUtils3D.h"
#include "engine/render/2d/Renderer2D.h"

namespace
{
float edgeFunction(const Vector2 &a, const Vector2 &b, const Vector2 &c)
{
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

void drawFilledTriangle(Renderer2D &renderer, const ProjectedVertex3D &v0, const ProjectedVertex3D &v1, const ProjectedVertex3D &v2, uint32_t color)
{
    if (!v0.visible || !v1.visible || !v2.visible)
        return;

    const float area = edgeFunction(v0.position, v1.position, v2.position);
    if (std::abs(area) < 0.0001f)
        return;

    const int minX = std::max(0, static_cast<int>(std::floor(std::min({v0.position.x, v1.position.x, v2.position.x}))));
    const int maxX = std::min(renderer.getWidth() - 1, static_cast<int>(std::ceil(std::max({v0.position.x, v1.position.x, v2.position.x}))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::min({v0.position.y, v1.position.y, v2.position.y}))));
    const int maxY = std::min(renderer.getHeight() - 1, static_cast<int>(std::ceil(std::max({v0.position.y, v1.position.y, v2.position.y}))));

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            const Vector2 p(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
            const float w0 = edgeFunction(v1.position, v2.position, p);
            const float w1 = edgeFunction(v2.position, v0.position, p);
            const float w2 = edgeFunction(v0.position, v1.position, p);

            const bool hasNegative = (w0 < 0.0f) || (w1 < 0.0f) || (w2 < 0.0f);
            const bool hasPositive = (w0 > 0.0f) || (w1 > 0.0f) || (w2 > 0.0f);
            if (hasNegative && hasPositive)
                continue;

            const float alpha = w0 / area;
            const float beta = w1 / area;
            const float gamma = w2 / area;
            const float depth = alpha * v0.depth + beta * v1.depth + gamma * v2.depth;
            renderer.drawPixelDepth(x, y, depth, color);
        }
    }
}
}

void SolidRenderer3D::drawCube(Renderer2D &renderer, const Camera3D &camera, const Transform3D &transform, float size) const
{
    const Matrix4 model = transform.getModelMatrix();
    const auto localVertices = CubeMesh3D::makeVertices(size);
    const auto projected = RenderUtils3D::projectVertices(localVertices, model, camera, renderer);

    for (const auto &triangle : CubeMesh3D::faces)
    {
        drawFilledTriangle(
            renderer,
            projected[triangle.indices[0]],
            projected[triangle.indices[1]],
            projected[triangle.indices[2]],
            triangle.color);
    }
}
