#pragma once
#include <X11/Xlib.h>

#include "platform/IWindow.h"

class X11Window : public IWindow
{
public:
    bool create(int width, int height, const char *title);
    void pollEvents();
    bool shouldClose() const;
    void present(const uint32_t *pixels);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    Display *display = nullptr;
    Window window;
    Atom wmDeleteMessage;

    bool running = true;
};