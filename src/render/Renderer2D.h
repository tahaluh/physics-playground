#pragma once
#include "platform/IWindow.h"
#include "Framebuffer.h"

class Renderer2D
{
public:
    Renderer2D(IWindow *window);
    void drawPixel(int x, int y, uint32_t color);
    void clear(uint32_t color);
    void present();

private:
    IWindow *window;
    Framebuffer framebuffer;
};