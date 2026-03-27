#pragma once
#include <X11/Xlib.h>
#include "../../platform/IWindow.h"

class X11Window : public IWindow
{
public:
    bool create(int width, int height, const char *title);
    void pollEvents();
    bool shouldClose() const;

private:
    Display *display = nullptr;
    Window window;
    Atom wmDeleteMessage;

    int width = 0;
    int height = 0;

    bool running = true;
};