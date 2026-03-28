#include "X11Window.h"
#include <iostream>
#include <X11/Xutil.h>

bool X11Window::create(int w, int h, const char *title)
{
    width = w;
    height = h;

    display = XOpenDisplay(nullptr);
    if (!display)
    {
        std::cerr << "Erro: nao foi possivel abrir o display X11\n";
        return false;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    window = XCreateSimpleWindow(
        display,
        root,
        100, 100,
        width, height,
        1,
        BlackPixel(display, screen),
        BlackPixel(display, screen));

    XSelectInput(display, window,
                 ExposureMask |
                     KeyPressMask |
                     StructureNotifyMask);

    XStoreName(display, window, title);

    wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);

    XMapWindow(display, window);

    XFlush(display);

    running = true;

    return true;
}

X11Window::~X11Window()
{
    if (display && window)
    {
        XDestroyWindow(display, window);
    }

    if (display)
    {
        XCloseDisplay(display);
    }
}

void X11Window::pollEvents()
{
    if (!display)
        return;
    while (XPending(display))
    {
        XEvent event;
        XNextEvent(display, &event);
        if (event.type == ClientMessage &&
            static_cast<Atom>(event.xclient.data.l[0]) == wmDeleteMessage)
        {
            running = false;
        }
        else if (event.type == ConfigureNotify)
        {
            width = event.xconfigure.width;
            height = event.xconfigure.height;
        }
    }
}

bool X11Window::shouldClose() const
{
    return !running;
}

void X11Window::present(const uint32_t *pixels)
{
    if (!display || !window || !pixels)
        return;

    XImage *image = XCreateImage(
        display,
        DefaultVisual(display, DefaultScreen(display)),
        DefaultDepth(display, DefaultScreen(display)),
        ZPixmap,
        0,
        (char *)pixels,
        width,
        height,
        32,
        0);

    if (!image)
        return;

    XPutImage(
        display,
        window,
        DefaultGC(display, DefaultScreen(display)),
        image,
        0, 0,
        0, 0,
        width,
        height);

    XFlush(display);
    image->data = nullptr;
    XDestroyImage(image);
}
