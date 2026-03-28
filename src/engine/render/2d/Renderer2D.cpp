#include "Renderer2D.h"

Renderer2D::Renderer2D(IWindow *window)
    : window(window), framebuffer(window->getWidth(), window->getHeight()) {}

void Renderer2D::drawPixel(int x, int y, uint32_t color)
{
    framebuffer.setPixel(x, y, color);
}

bool Renderer2D::drawPixelDepth(int x, int y, float z, uint32_t color)
{
    return framebuffer.setPixelDepth(x, y, z, color);
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

void Renderer2D::drawCircle(int cx, int cy, int radius, uint32_t color)
{
    // Midpoint circle algorithm
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y)
    {
        drawPixel(cx + x, cy + y, color);
        drawPixel(cx + y, cy + x, color);
        drawPixel(cx - y, cy + x, color);
        drawPixel(cx - x, cy + y, color);
        drawPixel(cx - x, cy - y, color);
        drawPixel(cx - y, cy - x, color);
        drawPixel(cx + y, cy - x, color);
        drawPixel(cx + x, cy - y, color);

        if (err <= 0)
        {
            ++y;
            err += 2 * y + 1;
        }
        if (err > 0)
        {
            --x;
            err -= 2 * x + 1;
        }
    }
}

void Renderer2D::fillCircle(int cx, int cy, int radius, uint32_t color)
{
    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            if (x * x + y * y <= radius * radius)
            {
                drawPixel(cx + x, cy + y, color);
            }
        }
    }
}

int Renderer2D::getWidth() const
{
    return framebuffer.getWidth();
}

int Renderer2D::getHeight() const
{
    return framebuffer.getHeight();
}

void Renderer2D::resize(int width, int height)
{
    framebuffer.resize(width, height);
}

void Renderer2D::drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color)
{
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

void Renderer2D::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color)
{
    auto swapVertex = [](int &xa, int &ya, int &xb, int &yb)
    {
        std::swap(xa, xb);
        std::swap(ya, yb);
    };

    // Sort by y
    if (y0 > y1)
        swapVertex(x0, y0, x1, y1);
    if (y1 > y2)
        swapVertex(x1, y1, x2, y2);
    if (y0 > y1)
        swapVertex(x0, y0, x1, y1);

    auto interpX = [](int xa, int ya, int xb, int yb, int y) -> float
    {
        if (ya == yb)
            return (float)xa;
        return xa + (xb - xa) * ((float)(y - ya) / (float)(yb - ya));
    };

    for (int y = y0; y <= y2; ++y)
    {
        float xA, xB;

        if (y < y1)
        {
            xA = interpX(x0, y0, x1, y1, y);
            xB = interpX(x0, y0, x2, y2, y);
        }
        else
        {
            xA = interpX(x1, y1, x2, y2, y);
            xB = interpX(x0, y0, x2, y2, y);
        }

        if (xA > xB)
            std::swap(xA, xB);

        for (int x = (int)xA; x <= (int)xB; ++x)
        {
            drawPixel(x, y, color);
        }
    }
}

void Renderer2D::clear(uint32_t color)
{
    framebuffer.clear(color);
}

void Renderer2D::clearDepth(float value)
{
    framebuffer.clearDepth(value);
}

void Renderer2D::present()
{
    window->present(framebuffer.data());
}
