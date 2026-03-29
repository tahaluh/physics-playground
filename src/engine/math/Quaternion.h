#pragma once

#include <cmath>

#include "engine/math/Matrix4.h"
#include "engine/math/Vector3.h"

struct Quaternion
{
    float w = 1.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Quaternion() = default;
    Quaternion(float w, float x, float y, float z)
        : w(w), x(x), y(y), z(z)
    {
    }

    static Quaternion identity()
    {
        return Quaternion();
    }

    static Quaternion fromAxisAngle(const Vector3 &axis, float radians)
    {
        const Vector3 normalizedAxis = axis.normalized();
        if (normalizedAxis.lengthSquared() == 0.0f)
        {
            return identity();
        }

        const float halfAngle = radians * 0.5f;
        const float sine = std::sin(halfAngle);
        return Quaternion(
            std::cos(halfAngle),
            normalizedAxis.x * sine,
            normalizedAxis.y * sine,
            normalizedAxis.z * sine);
    }

    static Quaternion fromEulerXYZ(const Vector3 &rotationRadians)
    {
        const Quaternion qx = fromAxisAngle(Vector3::right(), rotationRadians.x);
        const Quaternion qy = fromAxisAngle(Vector3::up(), rotationRadians.y);
        const Quaternion qz = fromAxisAngle(Vector3::forward(), rotationRadians.z);
        return (qz * qy * qx).normalized();
    }

    Quaternion operator*(const Quaternion &other) const
    {
        return Quaternion(
            w * other.w - x * other.x - y * other.y - z * other.z,
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w);
    }

    Quaternion normalized() const
    {
        const float magnitude = std::sqrt(w * w + x * x + y * y + z * z);
        if (magnitude <= 0.0f)
        {
            return identity();
        }

        return Quaternion(w / magnitude, x / magnitude, y / magnitude, z / magnitude);
    }

    Matrix4 toMatrix() const
    {
        const Quaternion q = normalized();
        const float xx = q.x * q.x;
        const float yy = q.y * q.y;
        const float zz = q.z * q.z;
        const float xy = q.x * q.y;
        const float xz = q.x * q.z;
        const float yz = q.y * q.z;
        const float wx = q.w * q.x;
        const float wy = q.w * q.y;
        const float wz = q.w * q.z;

        Matrix4 result = Matrix4::identity();
        result.m[0] = 1.0f - 2.0f * (yy + zz);
        result.m[1] = 2.0f * (xy - wz);
        result.m[2] = 2.0f * (xz + wy);

        result.m[4] = 2.0f * (xy + wz);
        result.m[5] = 1.0f - 2.0f * (xx + zz);
        result.m[6] = 2.0f * (yz - wx);

        result.m[8] = 2.0f * (xz - wy);
        result.m[9] = 2.0f * (yz + wx);
        result.m[10] = 1.0f - 2.0f * (xx + yy);
        return result;
    }
};
