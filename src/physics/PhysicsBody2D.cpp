#include "physics/PhysicsBody2D.h"
#include "physics/BorderBoxBody2D.h"
#include "physics/BorderCircleBody2D.h"
#include "physics/shapes/CircleShape.h"
#include "physics/shapes/RectShape.h"
#include "render/Renderer2D.h"

namespace
{
CircleShape *getCircleShape(PhysicsBody2D &body)
{
    return dynamic_cast<CircleShape *>(body.shape.get());
}
}

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

bool PhysicsBody2D::resolveDynamicCircleCollision(PhysicsBody2D &a, PhysicsBody2D &b)
{
    if (&a > &b)
        return false;

    auto *circleA = getCircleShape(a);
    auto *circleB = getCircleShape(b);
    if (!circleA || !circleB || a.isStatic() || b.isStatic())
        return false;

    Vector2 delta = b.position - a.position;
    float distanceSquared = delta.lengthSquared();
    float radiusSum = circleA->getRadius() + circleB->getRadius();
    if (distanceSquared <= 0.0f || distanceSquared > radiusSum * radiusSum)
        return false;

    float distance = std::sqrt(distanceSquared);
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

    const float combinedRestitution = std::min(a.restitution, b.restitution);
    float impulseMagnitude = -(1.0f + combinedRestitution) * velocityAlongNormal;
    impulseMagnitude /= (1.0f / a.mass) + (1.0f / b.mass);

    Vector2 impulse = normal * impulseMagnitude;
    a.velocity = a.velocity - impulse / a.mass;
    b.velocity = b.velocity + impulse / b.mass;
    return true;
}

bool PhysicsBody2D::resolveBorderCircleCollision(PhysicsBody2D &other, float stopThreshold)
{
    auto *borderCircle = dynamic_cast<BorderCircleBody2D *>(&other);
    auto *circle = getCircleShape(*this);
    if (!borderCircle || !circle)
        return false;

    const float innerRadius = borderCircle->getRadius() - circle->getRadius();
    Vector2 toBody = position - borderCircle->getCenter();
    float distanceSquared = toBody.lengthSquared();
    if (distanceSquared < innerRadius * innerRadius)
        return false;

    float distance = std::sqrt(std::max(distanceSquared, 0.0001f));
    Vector2 normal = toBody / distance;
    position = borderCircle->getCenter() + normal * innerRadius;
    velocity = velocity.reflect(normal) * restitution;
    applySurfaceFrictionAlongNormal(normal);

    if (stopThreshold > 0.0f && velocity.length() < stopThreshold)
    {
        velocity = Vector2::zero();
    }

    return true;
}

bool PhysicsBody2D::resolveBorderBoxCollision(PhysicsBody2D &other, float stopThreshold)
{
    auto *box = dynamic_cast<BorderBoxBody2D *>(&other);
    auto *circle = getCircleShape(*this);
    if (!box || !circle)
        return false;

    const float radius = circle->getRadius();
    Vector2 normal = Vector2::zero();
    bool reflected = false;

    if (position.x - radius <= box->left())
    {
        position.x = box->left() + radius;
        normal = Vector2(1.0f, 0.0f);
        reflected = true;
    }
    else if (position.x + radius >= box->right())
    {
        position.x = box->right() - radius;
        normal = Vector2(-1.0f, 0.0f);
        reflected = true;
    }

    if (position.y - radius <= box->top())
    {
        position.y = box->top() + radius;
        normal = Vector2(0.0f, 1.0f);
        reflected = true;
    }
    else if (position.y + radius >= box->bottom())
    {
        position.y = box->bottom() - radius;
        normal = Vector2(0.0f, -1.0f);
        reflected = true;
    }

    if (!reflected)
        return false;

    velocity = velocity.reflect(normal) * restitution;
    applySurfaceFrictionAlongNormal(normal);
    if (stopThreshold > 0.0f && velocity.length() < stopThreshold)
    {
        velocity = Vector2::zero();
    }

    return true;
}

void PhysicsBody2D::applySurfaceFrictionAlongNormal(const Vector2 &normal)
{
    if (surfaceFriction <= 0.0f)
        return;

    const float normalSpeed = velocity.dot(normal);
    const Vector2 tangentialVelocity = velocity - normal * normalSpeed;
    velocity -= tangentialVelocity * surfaceFriction;
}

bool PhysicsBody2D::resolveBorderCircleAxisInvertCollision(PhysicsBody2D &other)
{
    auto *borderCircle = dynamic_cast<BorderCircleBody2D *>(&other);
    auto *circle = getCircleShape(*this);
    if (!borderCircle || !circle)
        return false;

    const float innerRadius = borderCircle->getRadius() - circle->getRadius();
    Vector2 toBody = position - borderCircle->getCenter();
    float distanceSquared = toBody.lengthSquared();
    if (distanceSquared < innerRadius * innerRadius)
        return false;

    float distance = std::sqrt(std::max(distanceSquared, 0.0001f));
    Vector2 normal = toBody / distance;
    position = borderCircle->getCenter() + normal * innerRadius;

    if (std::abs(normal.x) > std::abs(normal.y))
    {
        velocity.x = -velocity.x * restitution;
    }
    else
    {
        velocity.y = -velocity.y * restitution;
    }

    return true;
}
