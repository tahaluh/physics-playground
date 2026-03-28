#include "physics/PhysicsBody2D.h"
#include "render/Renderer2D.h"
#include "physics/shapes/CircleShape.h"
#include "physics/shapes/RectShape.h"

void PhysicsBody2D::render(Renderer2D &renderer) const
{
    if (!shape)
        return;

    if (shape->getType() == ShapeType::Rect)
    {
        auto *rect = static_cast<RectShape *>(shape.get());
        renderer.drawRect(
            static_cast<int>(position.x),
            static_cast<int>(position.y),
            static_cast<int>(rect->getWidth()),
            static_cast<int>(rect->getHeight()),
            color);
        return;
    }

    if (shape->getType() == ShapeType::Circle)
    {
        auto *circ = static_cast<CircleShape *>(shape.get());
        renderer.drawCircle(
            static_cast<int>(position.x),
            static_cast<int>(position.y),
            static_cast<int>(circ->getRadius()),
            color);
    }
}
