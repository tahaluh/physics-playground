#include "X11Window.h"
#include <iostream>

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
        WhitePixel(display, screen));

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
    }
}

bool X11Window::shouldClose() const
{
    return !running;
}