#pragma once

#include "engine/math/Matrix4.h"
#include "engine/math/Vector3.h"

struct Transform3D
{
    Vector3 position = Vector3::zero();
    Vector3 rotation = Vector3::zero();
    Vector3 scale = Vector3::one();

    Matrix4 getTranslationMatrix() const
    {
        return Matrix4::translation(position);
    }

    Matrix4 getRotationMatrix() const
    {
        return Matrix4::rotationXYZ(rotation);
    }

    Matrix4 getScaleMatrix() const
    {
        return Matrix4::scale(scale);
    }

    Matrix4 getModelMatrix() const
    {
        return getTranslationMatrix() * getRotationMatrix() * getScaleMatrix();
    }

    Vector3 transformPoint(const Vector3 &point) const
    {
        return getModelMatrix().transformPoint(point);
    }
};
