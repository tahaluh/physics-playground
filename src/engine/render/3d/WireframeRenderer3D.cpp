#include "engine/render/3d/WireframeRenderer3D.h"

#include "engine/render/2d/Renderer2D.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/render/3d/Material3D.h"
#include "engine/render/3d/RenderUtils3D.h"
#include "engine/render/3d/mesh/Mesh3D.h"

void WireframeRenderer3D::drawMesh(Renderer2D &renderer, const Camera3D &camera, const Mesh3D &mesh, const Material3D &material, const Transform3D &transform) const
{
    if (!material.renderWireframe)
        return;

    const Matrix4 modelMatrix = transform.getModelMatrix();
    const auto projectedVertices = RenderUtils3D::projectVertices(mesh.vertices, modelMatrix, camera, renderer);

    for (const auto &edge : mesh.edges)
    {
        if (!projectedVertices[edge.start].visible || !projectedVertices[edge.end].visible)
            continue;

        renderer.drawLine(
            static_cast<int>(projectedVertices[edge.start].position.x),
            static_cast<int>(projectedVertices[edge.start].position.y),
            static_cast<int>(projectedVertices[edge.end].position.x),
            static_cast<int>(projectedVertices[edge.end].position.y),
            material.wireframeColor);
    }
}
