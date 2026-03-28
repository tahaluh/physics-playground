#pragma once
#include "engine/physics/2d/shapes/Shape.h"

class CircleShape : public Shape
{
public:
    CircleShape(float radius) : radius(radius) {}

    ShapeType getType() const override { return ShapeType::Circle; }
    bool contains(float x, float y) const override
    {
        return x * x + y * y <= radius * radius;
    }
    void getVertices(std::vector<Vector2> &out) const override
    {
        out.push_back(Vector2(0, 0));
    }
    float getRadius() const { return radius; }

    // Double dispatch
    bool collidesWith(const Shape &other, const Vector2 &thisPos, const Vector2 &otherPos) const override
    {
        return other.collidesWithCircle(*this, otherPos, thisPos);
    }
    bool collidesWithCircle(const CircleShape &circle, const Vector2 &thisPos, const Vector2 &circlePos) const override
    {
        float r = radius + circle.radius;
        float dx = thisPos.x - circlePos.x;
        float dy = thisPos.y - circlePos.y;
        float dist2 = dx * dx + dy * dy;
        return dist2 <= r * r;
    }
    bool collidesWithRect(const class RectShape &rect, const Vector2 &thisPos, const Vector2 &rectPos) const override;

private:
    float radius;
};
