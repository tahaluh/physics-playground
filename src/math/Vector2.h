#pragma once
#include <algorithm>
#include <cmath>

struct Vector2
{
    float x, y;
    Vector2(float x = 0, float y = 0) : x(x), y(y) {}

    static Vector2 zero() { return Vector2(0.0f, 0.0f); }

    Vector2 operator+(const Vector2 &other) const { return Vector2(x + other.x, y + other.y); }
    Vector2 operator-(const Vector2 &other) const { return Vector2(x - other.x, y - other.y); }
    Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
    Vector2 operator/(float scalar) const { return Vector2(x / scalar, y / scalar); }
    Vector2 &operator+=(const Vector2 &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    Vector2 &operator-=(const Vector2 &other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    Vector2 &operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    Vector2 &operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    float lengthSquared() const { return x * x + y * y; }
    float length() const { return std::sqrt(x * x + y * y); }
    float dot(const Vector2 &other) const { return x * other.x + y * other.y; }

    Vector2 normalized() const
    {
        float len = length();
        if (len == 0)
            return zero();
        return *this / len;
    }

    // Reflete este vetor em relação a uma normal
    Vector2 reflect(const Vector2 &normal) const
    {
        // v - 2*(v·n)*n
        return *this - normal * (2.0f * this->dot(normal));
    }

    // Clamp cada componente
    static float clamp(float v, float min, float max)
    {
        return std::max(min, std::min(v, max));
    }
    Vector2 clamped(const Vector2 &minV, const Vector2 &maxV) const
    {
        return Vector2(clamp(x, minV.x, maxV.x), clamp(y, minV.y, maxV.y));
    }
};
