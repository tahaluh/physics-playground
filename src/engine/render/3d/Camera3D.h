#pragma once

#include "engine/math/Matrix4.h"
#include "engine/math/Transform3D.h"
#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"

class Camera3D
{
public:
    Transform3D transform;
    float fovRadians = 60.0f * 3.14159265f / 180.0f;
    float aspectRatio = 4.0f / 3.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    Matrix4 getViewMatrix() const
    {
        return Matrix4::lookAt(transform.position, transform.position + getForward(), Vector3::up());
    }

    Matrix4 getProjectionMatrix() const
    {
        return Matrix4::perspective(fovRadians, aspectRatio, nearPlane, farPlane);
    }

    Vector3 getForward() const
    {
        return Matrix4::rotationXYZ(transform.rotation).transformVector(Vector3(0.0f, 0.0f, -1.0f)).normalized();
    }

    bool projectPoint(const Vector3 &worldPoint, int viewportWidth, int viewportHeight, Vector2 &screenPoint) const
    {
        Vector3 viewPoint = getViewMatrix().transformPoint(worldPoint);
        if (-viewPoint.z < nearPlane)
            return false;

        Vector3 ndcPoint = getProjectionMatrix().transformPoint(viewPoint);
        screenPoint.x = (ndcPoint.x * 0.5f + 0.5f) * static_cast<float>(viewportWidth);
        screenPoint.y = (1.0f - (ndcPoint.y * 0.5f + 0.5f)) * static_cast<float>(viewportHeight);
        return true;
    }
};
