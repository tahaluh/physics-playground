#pragma once

#include <array>
#include <cmath>

#include "engine/math/Vector3.h"

struct Matrix4
{
    std::array<float, 16> m = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f};

    static Matrix4 identity()
    {
        return Matrix4();
    }

    static Matrix4 translation(const Vector3 &translation)
    {
        Matrix4 result = identity();
        result.m[3] = translation.x;
        result.m[7] = translation.y;
        result.m[11] = translation.z;
        return result;
    }

    static Matrix4 scale(const Vector3 &scale)
    {
        Matrix4 result = identity();
        result.m[0] = scale.x;
        result.m[5] = scale.y;
        result.m[10] = scale.z;
        return result;
    }

    static Matrix4 rotationX(float radians)
    {
        Matrix4 result = identity();
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        result.m[5] = c;
        result.m[6] = -s;
        result.m[9] = s;
        result.m[10] = c;
        return result;
    }

    static Matrix4 rotationY(float radians)
    {
        Matrix4 result = identity();
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        result.m[0] = c;
        result.m[2] = s;
        result.m[8] = -s;
        result.m[10] = c;
        return result;
    }

    static Matrix4 rotationZ(float radians)
    {
        Matrix4 result = identity();
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        result.m[0] = c;
        result.m[1] = -s;
        result.m[4] = s;
        result.m[5] = c;
        return result;
    }

    static Matrix4 axisAngle(const Vector3 &axis, float radians)
    {
        const Vector3 normalizedAxis = axis.normalized();
        if (normalizedAxis.lengthSquared() == 0.0f)
        {
            return identity();
        }

        const float x = normalizedAxis.x;
        const float y = normalizedAxis.y;
        const float z = normalizedAxis.z;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        const float oneMinusC = 1.0f - c;

        Matrix4 result = identity();
        result.m[0] = c + x * x * oneMinusC;
        result.m[1] = x * y * oneMinusC - z * s;
        result.m[2] = x * z * oneMinusC + y * s;

        result.m[4] = y * x * oneMinusC + z * s;
        result.m[5] = c + y * y * oneMinusC;
        result.m[6] = y * z * oneMinusC - x * s;

        result.m[8] = z * x * oneMinusC - y * s;
        result.m[9] = z * y * oneMinusC + x * s;
        result.m[10] = c + z * z * oneMinusC;
        return result;
    }

    static Matrix4 rotationXYZ(const Vector3 &rotationRadians)
    {
        return rotationZ(rotationRadians.z) * rotationY(rotationRadians.y) * rotationX(rotationRadians.x);
    }

    static Matrix4 perspective(float fovRadians, float aspectRatio, float nearPlane, float farPlane)
    {
        Matrix4 result = {};
        result.m.fill(0.0f);

        const float tanHalfFov = std::tan(fovRadians * 0.5f);
        result.m[0] = 1.0f / (aspectRatio * tanHalfFov);
        result.m[5] = 1.0f / tanHalfFov;
        result.m[10] = -farPlane / (farPlane - nearPlane);
        result.m[11] = -(farPlane * nearPlane) / (farPlane - nearPlane);
        result.m[14] = -1.0f;
        return result;
    }

    static Matrix4 orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
    {
        Matrix4 result = {};
        result.m.fill(0.0f);
        result.m[0] = 2.0f / (right - left);
        result.m[3] = -(right + left) / (right - left);
        result.m[5] = 2.0f / (top - bottom);
        result.m[7] = -(top + bottom) / (top - bottom);
        result.m[10] = -2.0f / (farPlane - nearPlane);
        result.m[11] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result.m[15] = 1.0f;
        return result;
    }

    static Matrix4 orthographicVulkan(float left, float right, float bottom, float top, float nearPlane, float farPlane)
    {
        Matrix4 result = {};
        result.m.fill(0.0f);
        result.m[0] = 2.0f / (right - left);
        result.m[3] = -(right + left) / (right - left);
        result.m[5] = 2.0f / (top - bottom);
        result.m[7] = -(top + bottom) / (top - bottom);
        result.m[10] = -1.0f / (farPlane - nearPlane);
        result.m[11] = -nearPlane / (farPlane - nearPlane);
        result.m[15] = 1.0f;
        return result;
    }

    static Matrix4 lookAt(const Vector3 &eye, const Vector3 &target, const Vector3 &up)
    {
        const Vector3 forward = (target - eye).normalized();
        const Vector3 right = forward.cross(up).normalized();
        const Vector3 correctedUp = right.cross(forward);

        Matrix4 result = identity();
        result.m[0] = right.x;
        result.m[1] = right.y;
        result.m[2] = right.z;
        result.m[3] = -right.dot(eye);

        result.m[4] = correctedUp.x;
        result.m[5] = correctedUp.y;
        result.m[6] = correctedUp.z;
        result.m[7] = -correctedUp.dot(eye);

        result.m[8] = -forward.x;
        result.m[9] = -forward.y;
        result.m[10] = -forward.z;
        result.m[11] = forward.dot(eye);
        return result;
    }

    Matrix4 operator*(const Matrix4 &other) const
    {
        Matrix4 result = {};
        result.m.fill(0.0f);

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                for (int k = 0; k < 4; ++k)
                {
                    result.m[row * 4 + col] += m[row * 4 + k] * other.m[k * 4 + col];
                }
            }
        }

        return result;
    }

    Vector3 transformPoint(const Vector3 &point) const
    {
        const float x = m[0] * point.x + m[1] * point.y + m[2] * point.z + m[3];
        const float y = m[4] * point.x + m[5] * point.y + m[6] * point.z + m[7];
        const float z = m[8] * point.x + m[9] * point.y + m[10] * point.z + m[11];
        const float w = m[12] * point.x + m[13] * point.y + m[14] * point.z + m[15];

        if (w != 0.0f && w != 1.0f)
        {
            return Vector3(x / w, y / w, z / w);
        }

        return Vector3(x, y, z);
    }

    Vector3 transformVector(const Vector3 &vector) const
    {
        return Vector3(
            m[0] * vector.x + m[1] * vector.y + m[2] * vector.z,
            m[4] * vector.x + m[5] * vector.y + m[6] * vector.z,
            m[8] * vector.x + m[9] * vector.y + m[10] * vector.z);
    }
};
