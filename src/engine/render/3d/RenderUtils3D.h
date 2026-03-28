#pragma once

#include <array>

#include "engine/math/Matrix4.h"
#include "engine/math/Vector3.h"
#include "engine/render/3d/ProjectedVertex3D.h"
#include "engine/render/2d/Renderer2D.h"

class Camera3D;

namespace RenderUtils3D
{
template <size_t N>
std::array<ProjectedVertex3D, N> projectVertices(
    const std::array<Vector3, N> &localVertices,
    const Matrix4 &model,
    const Camera3D &camera,
    const Renderer2D &renderer)
{
    const Matrix4 view = camera.getViewMatrix();
    const Matrix4 projection = camera.getProjectionMatrix();
    std::array<ProjectedVertex3D, N> projected = {};

    for (size_t i = 0; i < localVertices.size(); ++i)
    {
        const Vector3 world = model.transformPoint(localVertices[i]);
        const Vector3 viewPoint = view.transformPoint(world);
        if (-viewPoint.z < camera.nearPlane)
        {
            projected[i].visible = false;
            continue;
        }

        const Vector3 clip = projection.transformPoint(viewPoint);
        projected[i].visible = true;
        projected[i].position.x = (clip.x * 0.5f + 0.5f) * static_cast<float>(renderer.getWidth());
        projected[i].position.y = (1.0f - (clip.y * 0.5f + 0.5f)) * static_cast<float>(renderer.getHeight());
        projected[i].depth = clip.z * 0.5f + 0.5f;
    }

    return projected;
}
}
