#include "engine/render/3d/WireframeRenderer3D.h"

#include "engine/render/3d/Camera3D.h"
#include "engine/render/3d/CubeMesh3D.h"
#include "engine/render/3d/RenderUtils3D.h"
#include "engine/render/2d/Renderer2D.h"

void WireframeRenderer3D::drawCube(Renderer2D &renderer, const Camera3D &camera, const Transform3D &transform, float size, uint32_t color) const
{
    const Matrix4 modelMatrix = transform.getModelMatrix();
    const auto localVertices = CubeMesh3D::makeVertices(size);
    const auto projectedVertices = RenderUtils3D::projectVertices(localVertices, modelMatrix, camera, renderer);

    for (const auto &edge : CubeMesh3D::edges)
    {
        if (!projectedVertices[edge[0]].visible || !projectedVertices[edge[1]].visible)
            continue;

        renderer.drawLine(
            static_cast<int>(projectedVertices[edge[0]].position.x),
            static_cast<int>(projectedVertices[edge[0]].position.y),
            static_cast<int>(projectedVertices[edge[1]].position.x),
            static_cast<int>(projectedVertices[edge[1]].position.y),
            color);
    }
}
