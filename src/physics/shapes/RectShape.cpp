#include "physics/shapes/RectShape.h"
#include "physics/shapes/CircleShape.h"
#include <algorithm>

// Rect vs Circle (delegates to CircleShape)
bool RectShape::collidesWithCircle(const CircleShape &circle, const Vector2 &rectPos, const Vector2 &circlePos) const
{
    return circle.collidesWithRect(*this, circlePos, rectPos);
}
