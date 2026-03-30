#pragma once

#include "engine/math/Matrix4.h"
#include "engine/math/Transform.h"
#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"

class Camera
{
public:
    Transform transform;
    float fovRadians = 60.0f * 3.14159265f / 180.0f;
    float aspectRatio = 4.0f / 3.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;

    Matrix4 getViewMatrix() const
    {
        return Matrix4::lookAt(transform.position, transform.position + getForward(), getUp());
    }

    Matrix4 getProjectionMatrix() const
    {
        return Matrix4::perspective(fovRadians, aspectRatio, nearPlane, farPlane);
    }

    void setAspectRatio(float newAspectRatio)
    {
        if (newAspectRatio > 0.0f)
        {
            aspectRatio = newAspectRatio;
        }
    }

    void setViewport(int x, int y, int width, int height)
    {
        viewportX = x;
        viewportY = y;
        viewportWidth = width;
        viewportHeight = height;
        setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    }

    Vector3 getForward() const
    {
        return transform.rotation.toMatrix().transformVector(Vector3(0.0f, 0.0f, -1.0f)).normalized();
    }

    Vector3 getRight() const
    {
        return transform.rotation.toMatrix().transformVector(Vector3::right()).normalized();
    }

    Vector3 getUp() const
    {
        return getRight().cross(getForward()).normalized();
    }

    bool projectPoint(const Vector3 &worldPoint, int viewportWidth, int viewportHeight, Vector2 &screenPoint) const
    {
        const int resolvedViewportX = this->viewportWidth > 0 ? this->viewportX : 0;
        const int resolvedViewportY = this->viewportHeight > 0 ? this->viewportY : 0;
        const int resolvedViewportWidth = this->viewportWidth > 0 ? this->viewportWidth : viewportWidth;
        const int resolvedViewportHeight = this->viewportHeight > 0 ? this->viewportHeight : viewportHeight;

        Vector3 viewPoint = getViewMatrix().transformPoint(worldPoint);
        if (-viewPoint.z < nearPlane)
            return false;

        Vector3 ndcPoint = getProjectionMatrix().transformPoint(viewPoint);
        if (ndcPoint.z < 0.0f || ndcPoint.z > 1.0f)
            return false;

        screenPoint.x = static_cast<float>(resolvedViewportX) +
                        (ndcPoint.x * 0.5f + 0.5f) * static_cast<float>(resolvedViewportWidth);
        screenPoint.y = static_cast<float>(resolvedViewportY) +
                        (1.0f - (ndcPoint.y * 0.5f + 0.5f)) * static_cast<float>(resolvedViewportHeight);
        return true;
    }
};
