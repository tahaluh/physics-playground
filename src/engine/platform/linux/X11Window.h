#pragma once
#include <X11/Xlib.h>

#include "engine/platform/IWindow.h"

class X11Window : public IWindow
{
public:
    bool create(int width, int height, const char *title);
    void pollEvents();
    bool shouldClose() const;
    void present(const uint32_t *pixels);
    ~X11Window() override;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    Display *display = nullptr;
    Window window = 0;
    Atom wmDeleteMessage = 0;

    bool running = true;
};
