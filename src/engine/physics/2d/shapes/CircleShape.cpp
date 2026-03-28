#include "engine/physics/2d/shapes/CircleShape.h"
#include "engine/physics/2d/shapes/RectShape.h"
#include <algorithm>

// Circle vs Rect
bool CircleShape::collidesWithRect(const RectShape &rect, const Vector2 &circlePos, const Vector2 &rectPos) const
{
    float cx = circlePos.x;
    float cy = circlePos.y;
    float rx = rectPos.x;
    float ry = rectPos.y;
    float rw = rect.getWidth();
    float rh = rect.getHeight();
    float closestX = std::max(rx, std::min(cx, rx + rw));
    float closestY = std::max(ry, std::min(cy, ry + rh));
    float dx = cx - closestX;
    float dy = cy - closestY;
    return (dx * dx + dy * dy) <= (getRadius() * getRadius());
}
