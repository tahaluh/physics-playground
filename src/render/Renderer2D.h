#pragma once
#include "platform/IWindow.h"
#include "Framebuffer.h"

class Renderer2D
{
public:
    Renderer2D(IWindow *window);

    // Draw
    void drawPixel(int x, int y, uint32_t color);
    void drawLine(int x0, int y0, int x1, int y1, uint32_t color);

    void drawRect(int x, int y, int width, int height, uint32_t color);
    void fillRect(int x, int y, int width, int height, uint32_t color);

    void drawCircle(int cx, int cy, int radius, uint32_t color);
    void fillCircle(int cx, int cy, int radius, uint32_t color);

    void clear(uint32_t color);
    void present();

private:
    IWindow *window;
    Framebuffer framebuffer;
};