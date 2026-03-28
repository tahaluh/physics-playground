#include "engine/render/3d/SceneRenderer3D.h"

#include "engine/render/2d/Renderer2D.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/render/3d/RenderUtils3D.h"
#include "engine/scene/3d/Scene3D.h"

void SceneRenderer3D::render(Renderer2D &renderer, const Camera3D &camera, const Scene3D &scene) const
{
    for (const auto &entity : scene.getEntities())
    {
        const Matrix4 modelMatrix = entity.transform.getModelMatrix();
        const auto projectedVertices = RenderUtils3D::projectVertices(entity.mesh.vertices, modelMatrix, camera, renderer);

        solidRenderer.drawProjectedMesh(renderer, entity.mesh, entity.material, projectedVertices);
        wireframeRenderer.drawProjectedMesh(renderer, entity.mesh, entity.material, projectedVertices);
    }
}
