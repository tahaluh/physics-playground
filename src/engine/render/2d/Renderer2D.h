#pragma once
#include "engine/platform/IWindow.h"
#include "engine/render/Framebuffer.h"

class Renderer2D
{
public:
    Renderer2D(IWindow *window);

    // Draw
    void drawPixel(int x, int y, uint32_t color);
    bool drawPixelDepth(int x, int y, float z, uint32_t color);
    void drawLine(int x0, int y0, int x1, int y1, uint32_t color);

    void drawRect(int x, int y, int width, int height, uint32_t color);
    void fillRect(int x, int y, int width, int height, uint32_t color);

    void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);

    void drawCircle(int cx, int cy, int radius, uint32_t color);
    void fillCircle(int cx, int cy, int radius, uint32_t color);

    int getWidth() const;
    int getHeight() const;

    void resize(int width, int height);
    void clear(uint32_t color);
    void clearDepth(float value = 1.0e9f);
    void present();

private:
    IWindow *window;
    Framebuffer framebuffer;
};
