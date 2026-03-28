#pragma once
#include "engine/physics/2d/shapes/Shape.h"

class RectShape : public Shape
{
public:
    RectShape(float w, float h) : width(w), height(h) {}

    ShapeType getType() const override { return ShapeType::Rect; }
    bool contains(float x, float y) const override
    {
        return x >= 0 && x <= width && y >= 0 && y <= height;
    }
    void getVertices(std::vector<Vector2> &out) const override
    {
        out.push_back(Vector2(0, 0));
        out.push_back(Vector2(width, 0));
        out.push_back(Vector2(width, height));
        out.push_back(Vector2(0, height));
    }
    float getWidth() const { return width; }
    float getHeight() const { return height; }

    // Double dispatch
    bool collidesWith(const Shape &other, const Vector2 &thisPos, const Vector2 &otherPos) const override
    {
        return other.collidesWithRect(*this, otherPos, thisPos);
    }
    bool collidesWithCircle(const CircleShape &circle, const Vector2 &thisPos, const Vector2 &circlePos) const override;
    bool collidesWithRect(const RectShape &rect, const Vector2 &thisPos, const Vector2 &rectPos) const override
    {
        float al = thisPos.x;
        float ar = al + width;
        float at = thisPos.y;
        float ab = at + height;
        float bl = rectPos.x;
        float br = bl + rect.width;
        float bt = rectPos.y;
        float bb = bt + rect.height;
        return al < br && ar > bl && at < bb && ab > bt;
    }

private:
    float width, height;
};
