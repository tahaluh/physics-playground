#pragma once
#include <vector>
#include "math/Vector2.h"

enum class ShapeType
{
    Circle,
    Rect,
    Polygon
};

class Shape
{
public:
    virtual ShapeType getType() const = 0;
    virtual bool contains(float x, float y) const = 0;
    virtual void getVertices(std::vector<Vector2> &out) const = 0;
    virtual bool collidesWith(const Shape &other, const Vector2 &thisPos, const Vector2 &otherPos) const = 0;
    virtual bool collidesWithCircle(const class CircleShape &circle, const Vector2 &thisPos, const Vector2 &circlePos) const = 0;
    virtual bool collidesWithRect(const class RectShape &rect, const Vector2 &thisPos, const Vector2 &rectPos) const = 0;
    virtual ~Shape() = default;
};
