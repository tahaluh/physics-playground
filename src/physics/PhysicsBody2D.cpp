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
    return dynamic_cast<CircleShape *>(body.getShape());
}
}

void PhysicsBody2D::render(Renderer2D &renderer) const
{
    if (!getShape())
        return;

    if (getShape()->getType() == ShapeType::Rect)
    {
        auto *rect = static_cast<const RectShape *>(getShape());
        renderer.drawRect(
            static_cast<int>(position.x),
            static_cast<int>(position.y),
            static_cast<int>(rect->getWidth()),
            static_cast<int>(rect->getHeight()),
            color);
        return;
    }

    if (getShape()->getType() == ShapeType::Circle)
    {
        auto *circ = static_cast<const CircleShape *>(getShape());
        renderer.drawCircle(
            static_cast<int>(position.x),
            static_cast<int>(position.y),
            static_cast<int>(circ->getRadius()),
            color);
    }
}

bool PhysicsBody2D::resolveBorderCircleCollision(const Contact2D &contact, float stopThreshold)
{
    auto *borderCircle = dynamic_cast<BorderCircleBody2D *>(contact.other);
    auto *circle = getCircleShape(*this);
    if (!borderCircle || !circle)
        return false;

    const float innerRadius = borderCircle->getRadius() - circle->getRadius();
    Vector2 normal = contact.normal;
    if (normal.lengthSquared() <= 0.0f)
        return false;
    position = borderCircle->getCenter() + normal * innerRadius;
    velocity = velocity.reflect(normal) * material.restitution;
    applySurfaceFrictionAlongNormal(normal);

    if (stopThreshold > 0.0f && velocity.length() < stopThreshold)
    {
        velocity = Vector2::zero();
    }

    return true;
}

bool PhysicsBody2D::resolveBorderBoxCollision(const Contact2D &contact, float stopThreshold)
{
    auto *box = dynamic_cast<BorderBoxBody2D *>(contact.other);
    auto *circle = getCircleShape(*this);
    if (!box || !circle)
        return false;

    const float radius = circle->getRadius();
    Vector2 normal = contact.normal;
    if (normal.lengthSquared() <= 0.0f)
        return false;

    if (normal.x > 0.0f)
        position.x = box->left() + radius;
    else if (normal.x < 0.0f)
        position.x = box->right() - radius;

    if (normal.y > 0.0f)
        position.y = box->top() + radius;
    else if (normal.y < 0.0f)
        position.y = box->bottom() - radius;

    velocity = velocity.reflect(normal) * material.restitution;
    applySurfaceFrictionAlongNormal(normal);
    if (stopThreshold > 0.0f && velocity.length() < stopThreshold)
    {
        velocity = Vector2::zero();
    }

    return true;
}

void PhysicsBody2D::applySurfaceFrictionAlongNormal(const Vector2 &normal)
{
    if (material.surfaceFriction <= 0.0f)
        return;

    const float normalSpeed = velocity.dot(normal);
    const Vector2 tangentialVelocity = velocity - normal * normalSpeed;
    velocity -= tangentialVelocity * material.surfaceFriction;
}

bool PhysicsBody2D::resolveBorderCircleAxisInvertCollision(const Contact2D &contact)
{
    auto *borderCircle = dynamic_cast<BorderCircleBody2D *>(contact.other);
    auto *circle = getCircleShape(*this);
    if (!borderCircle || !circle)
        return false;

    const float innerRadius = borderCircle->getRadius() - circle->getRadius();
    Vector2 normal = contact.normal;
    if (normal.lengthSquared() <= 0.0f)
        return false;
    position = borderCircle->getCenter() + normal * innerRadius;

    if (std::abs(normal.x) > std::abs(normal.y))
    {
        velocity.x = -velocity.x * material.restitution;
    }
    else
    {
        velocity.y = -velocity.y * material.restitution;
    }

    return true;
}
