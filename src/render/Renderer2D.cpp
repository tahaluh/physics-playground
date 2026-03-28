#include "Renderer2D.h"

Renderer2D::Renderer2D(IWindow *window)
    : window(window), framebuffer(window->getWidth(), window->getHeight()) {}

void Renderer2D::drawPixel(int x, int y, uint32_t color)
{
    framebuffer.setPixel(x, y, color);
}

void Renderer2D::clear(uint32_t color)
{
    framebuffer.clear(color);
}

void Renderer2D::present()
{
    window->present(framebuffer.data());
}
