#include "render/DebugRenderer2D.h"

#include "physics/Manifold2D.h"
#include "render/Renderer2D.h"

void DebugRenderer2D::drawContacts(Renderer2D &renderer, const std::vector<Manifold2D> &manifolds) const
{
    constexpr uint32_t contactColor = 0xFFFF4444;
    constexpr uint32_t normalColor = 0xFF44FF44;
    constexpr int normalLength = 20;

    for (const auto &manifold : manifolds)
    {
        for (size_t i = 0; i < manifold.contactCount; ++i)
        {
            const Vector2 &point = manifold.contactPoints[i];
            renderer.fillCircle(static_cast<int>(point.x), static_cast<int>(point.y), 2, contactColor);

            Vector2 normalEnd = point + manifold.normal * static_cast<float>(normalLength);
            renderer.drawLine(
                static_cast<int>(point.x),
                static_cast<int>(point.y),
                static_cast<int>(normalEnd.x),
                static_cast<int>(normalEnd.y),
                normalColor);
        }
    }
}
