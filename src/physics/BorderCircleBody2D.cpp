#include "physics/BorderCircleBody2D.h"
#include "render/Renderer2D.h"

void BorderCircleBody2D::render(Renderer2D &renderer) const
{
    renderer.drawCircle(
        static_cast<int>(position.x),
        static_cast<int>(position.y),
        static_cast<int>(radius),
        0xFFFFFFFF);
}
