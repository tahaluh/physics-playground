#include "physics/PhysicsBody2D.h"
#include "physics/shapes/CircleShape.h"
#include "physics/shapes/RectShape.h"
#include "render/Renderer2D.h"

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

bool PhysicsBody2D::resolveDynamicCircleCollision(PhysicsBody2D &a, PhysicsBody2D &b, float restitution)
{
    if (&a > &b)
        return false;

    auto *circleA = dynamic_cast<CircleShape *>(a.shape.get());
    auto *circleB = dynamic_cast<CircleShape *>(b.shape.get());
    if (!circleA || !circleB || a.isStatic() || b.isStatic())
        return false;

    Vector2 delta = b.position - a.position;
    float distance = delta.length();
    float radiusSum = circleA->getRadius() + circleB->getRadius();
    if (distance <= 0.0f || distance > radiusSum)
        return false;

    Vector2 normal = delta / distance;
    float penetration = radiusSum - distance;
    float totalMass = a.mass + b.mass;
    if (totalMass <= 0.0f)
        return false;

    a.position = a.position - normal * (penetration * (b.mass / totalMass));
    b.position = b.position + normal * (penetration * (a.mass / totalMass));

    Vector2 relativeVelocity = b.velocity - a.velocity;
    float velocityAlongNormal = relativeVelocity.dot(normal);
    if (velocityAlongNormal > 0.0f)
        return true;

    float impulseMagnitude = -(1.0f + restitution) * velocityAlongNormal;
    impulseMagnitude /= (1.0f / a.mass) + (1.0f / b.mass);

    Vector2 impulse = normal * impulseMagnitude;
    a.velocity = a.velocity - impulse / a.mass;
    b.velocity = b.velocity + impulse / b.mass;
    return true;
}
