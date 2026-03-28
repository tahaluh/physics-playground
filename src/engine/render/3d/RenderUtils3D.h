#pragma once

#include <array>
#include <vector>

#include "engine/math/Matrix4.h"
#include "engine/math/Vector3.h"
#include "engine/render/2d/Renderer2D.h"
#include "engine/render/3d/ProjectedVertex3D.h"

class Camera3D;

namespace RenderUtils3D
{
inline ProjectedVertex3D projectVertex(
    const Vector3 &localVertex,
    const Matrix4 &model,
    const Camera3D &camera,
    const Renderer2D &renderer)
{
    const Matrix4 view = camera.getViewMatrix();
    const Matrix4 projection = camera.getProjectionMatrix();

    ProjectedVertex3D projected;
    projected.worldPosition = model.transformPoint(localVertex);
    projected.viewPosition = view.transformPoint(projected.worldPosition);
    if (-projected.viewPosition.z < camera.nearPlane)
    {
        projected.visible = false;
        return projected;
    }

    const Vector3 ndc = projection.transformPoint(projected.viewPosition);
    projected.visible = true;
    projected.position.x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(renderer.getWidth());
    projected.position.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(renderer.getHeight());
    projected.depth = ndc.z * 0.5f + 0.5f;
    return projected;
}

template <size_t N>
std::array<ProjectedVertex3D, N> projectVertices(
    const std::array<Vector3, N> &localVertices,
    const Matrix4 &model,
    const Camera3D &camera,
    const Renderer2D &renderer)
{
    std::array<ProjectedVertex3D, N> projected = {};
    for (size_t i = 0; i < localVertices.size(); ++i)
    {
        projected[i] = projectVertex(localVertices[i], model, camera, renderer);
    }
    return projected;
}

inline std::vector<ProjectedVertex3D> projectVertices(
    const std::vector<Vector3> &localVertices,
    const Matrix4 &model,
    const Camera3D &camera,
    const Renderer2D &renderer)
{
    std::vector<ProjectedVertex3D> projected(localVertices.size());
    for (size_t i = 0; i < localVertices.size(); ++i)
    {
        projected[i] = projectVertex(localVertices[i], model, camera, renderer);
    }
    return projected;
}
}
