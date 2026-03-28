#include "Renderer2D.h"

Renderer2D::Renderer2D(IWindow *window)
    : window(window), framebuffer(window->getWidth(), window->getHeight()) {}

void Renderer2D::drawPixel(int x, int y, uint32_t color)
{
    framebuffer.setPixel(x, y, color);
}

void Renderer2D::drawLine(int x0, int y0, int x1, int y1, uint32_t color)
{
    // Bresenham's line algorithm
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        int err2 = err * 2;
        if (err2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (err2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void Renderer2D::drawRect(int x, int y, int width, int height, uint32_t color)
{
    drawLine(x, y, x + width, y, color);
    drawLine(x + width, y, x + width, y + height, color);
    drawLine(x + width, y + height, x, y + height, color);
    drawLine(x, y + height, x, y, color);
}

void Renderer2D::fillRect(int x, int y, int width, int height, uint32_t color)
{
    for (int j = 0; j <= height; ++j)
    {
        for (int i = 0; i <= width; ++i)
        {
            drawPixel(x + i, y + j, color);
        }
    }
}

void Renderer2D::clear(uint32_t color)
{
    framebuffer.clear(color);
}

void Renderer2D::present()
{
    window->present(framebuffer.data());
}
