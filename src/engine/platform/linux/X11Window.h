#pragma once
#include <X11/Xlib.h>
#include <vector>

#include "engine/platform/IWindow.h"

class X11Window : public IWindow
{
public:
    bool create(int width, int height, const char *title);
    void pollEvents();
    bool shouldClose() const;
    void present(const uint32_t *pixels);
    void setMouseCaptured(bool captured) override;
    ~X11Window() override;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    void recenterCursor();
    bool recreateBackbuffer();

    Display *display = nullptr;
    Window window = 0;
    Atom wmDeleteMessage = 0;
    Cursor invisibleCursor = 0;
    XImage *image = nullptr;
    std::vector<uint32_t> imagePixels;

    bool running = true;
    bool mouseCaptured = false;
    bool mouseCaptureRequested = false;
    bool suppressCenteredMotion = false;
};
