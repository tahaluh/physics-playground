#include "X11Window.h"

#include <X11/Xutil.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vulkan/vulkan_xlib.h>

#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/platform/linux/input/X11InputTranslator.h"

void X11Window::requestInitialFocus()
{
    if (!display || !window)
    {
        return;
    }

    XWindowAttributes attributes;
    if (XGetWindowAttributes(display, window, &attributes) == 0)
    {
        return;
    }

    if (attributes.map_state != IsViewable)
    {
        return;
    }

    XRaiseWindow(display, window);
    XSetInputFocus(display, window, RevertToParent, CurrentTime);
    XFlush(display);
    focusRequested = true;
}

void X11Window::destroyPresentationResources()
{
    if (image)
    {
        image->data = nullptr;
        XDestroyImage(image);
        image = nullptr;
    }

    if (display && dbeBackBuffer)
    {
        XdbeDeallocateBackBufferName(display, dbeBackBuffer);
        dbeBackBuffer = 0;
    }
}

bool X11Window::recreateBackbuffer()
{
    if (!display || !window || width <= 0 || height <= 0)
    {
        return false;
    }

    destroyPresentationResources();

    imagePixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
    image = XCreateImage(
        display,
        DefaultVisual(display, DefaultScreen(display)),
        DefaultDepth(display, DefaultScreen(display)),
        ZPixmap,
        0,
        reinterpret_cast<char *>(imagePixels.data()),
        width,
        height,
        32,
        0);

    if (!image)
    {
        return false;
    }

    if (hasDbe)
    {
        dbeBackBuffer = XdbeAllocateBackBufferName(display, window, XdbeUndefined);
        if (!dbeBackBuffer)
        {
            hasDbe = false;
        }
    }

    return true;
}

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

    XWMHints hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.flags = InputHint;
    hints.input = True;
    XSetWMHints(display, window, &hints);

    XSelectInput(display, window,
                 ExposureMask |
                     ButtonPressMask |
                     FocusChangeMask |
                     KeyPressMask |
                     KeyReleaseMask |
                     PointerMotionMask |
                     StructureNotifyMask);

    XStoreName(display, window, title);

    wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);

    XMapRaised(display, window);
    XFlush(display);
    XSync(display, False);
    requestInitialFocus();

    int dbeMajor = 0;
    int dbeMinor = 0;
    hasDbe = XdbeQueryExtension(display, &dbeMajor, &dbeMinor) != 0;

    running = true;
    return recreateBackbuffer();
}

X11Window::~X11Window()
{
    if (display && mouseCaptured)
    {
        XUngrabPointer(display, CurrentTime);
    }

    if (display && invisibleCursor)
    {
        XFreeCursor(display, invisibleCursor);
    }

    destroyPresentationResources();

    if (display && window)
    {
        XDestroyWindow(display, window);
    }

    if (display)
    {
        XCloseDisplay(display);
    }
}

void X11Window::setMouseCaptured(bool captured)
{
    if (!display || !window)
    {
        return;
    }

    mouseCaptureRequested = captured;

    if (mouseCaptured == captured)
    {
        return;
    }

    if (captured)
    {
        Input::resetMouseDelta();
        char bitmapData[1] = {0};
        Pixmap blankPixmap = XCreateBitmapFromData(display, window, bitmapData, 1, 1);
        XColor dummyColor;
        std::memset(&dummyColor, 0, sizeof(dummyColor));

        if (!invisibleCursor)
        {
            invisibleCursor = XCreatePixmapCursor(display, blankPixmap, blankPixmap, &dummyColor, &dummyColor, 0, 0);
        }

        XFreePixmap(display, blankPixmap);
        XDefineCursor(display, window, invisibleCursor);

        XRaiseWindow(display, window);
        XSync(display, False);

        const int grabResult = XGrabPointer(
            display,
            window,
            False,
            ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
            GrabModeAsync,
            GrabModeAsync,
            window,
            invisibleCursor,
            CurrentTime);

        if (grabResult == GrabSuccess)
        {
            mouseCaptured = true;
            recenterCursor();
        }
        else
        {
            mouseCaptured = false;
            XUndefineCursor(display, window);
        }
    }
    else
    {
        Input::resetMouseDelta();
        XUngrabPointer(display, CurrentTime);
        XUndefineCursor(display, window);
        mouseCaptured = false;
    }

    XFlush(display);
}

void X11Window::setTitle(const char *title)
{
    if (!display || !window || !title)
    {
        return;
    }

    XStoreName(display, window, title);
    XFlush(display);
}

const char *const *X11Window::getRequiredVulkanInstanceExtensions(uint32_t &count) const
{
    static const char *extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    };

    count = static_cast<uint32_t>(sizeof(extensions) / sizeof(extensions[0]));
    return extensions;
}

bool X11Window::createVulkanSurface(VkInstance instance, VkSurfaceKHR &surface) const
{
    if (!display || !window)
    {
        return false;
    }

    VkXlibSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.dpy = display;
    createInfo.window = window;

    return vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface) == VK_SUCCESS;
}

void *X11Window::getNativeDisplayHandle()
{
    return display;
}

unsigned long X11Window::getNativeWindowHandle()
{
    return window;
}

void X11Window::recenterCursor()
{
    if (!display || !window)
    {
        return;
    }

    suppressCenteredMotion = true;
    Input::setMousePosition(static_cast<float>(width) * 0.5f, static_cast<float>(height) * 0.5f);
    XWarpPointer(display, None, window, 0, 0, 0, 0, width / 2, height / 2);
    XFlush(display);
}

void X11Window::pollEvents()
{
    if (!display)
        return;

    if (mouseCaptureRequested && !mouseCaptured)
    {
        setMouseCaptured(true);
    }

    if (!focusRequested)
    {
        requestInitialFocus();
    }

    while (XPending(display))
    {
        XEvent event;
        XNextEvent(display, &event);

        if (event.type == ClientMessage &&
            static_cast<Atom>(event.xclient.data.l[0]) == wmDeleteMessage)
        {
            running = false;
        }
        else if (event.type == FocusOut)
        {
            if (mouseCaptured)
            {
                XUngrabPointer(display, CurrentTime);
                XUndefineCursor(display, window);
                mouseCaptured = false;
            }
        }
        else if (event.type == FocusIn)
        {
            focusRequested = true;
            if (mouseCaptureRequested && !mouseCaptured)
            {
                setMouseCaptured(true);
            }
        }
        else if (event.type == ButtonPress)
        {
            if (mouseCaptureRequested && !mouseCaptured)
            {
                setMouseCaptured(true);
            }
        }
        else if (event.type == KeyPress || event.type == KeyRelease)
        {
            const bool isDown = event.type == KeyPress;
            EngineKeyCode key;
            if (X11InputTranslator::tryTranslateKey(event.xkey, key))
            {
                Input::setKeyState(key, isDown);
            }
        }
        else if (event.type == MotionNotify)
        {
            const float mouseX = static_cast<float>(event.xmotion.x);
            const float mouseY = static_cast<float>(event.xmotion.y);

            if (mouseCaptured)
            {
                const float centerX = static_cast<float>(width) * 0.5f;
                const float centerY = static_cast<float>(height) * 0.5f;
                const float dx = mouseX - centerX;
                const float dy = mouseY - centerY;

                if (suppressCenteredMotion)
                {
                    if (std::abs(dx) <= 1.0f && std::abs(dy) <= 1.0f)
                    {
                        suppressCenteredMotion = false;
                    }

                    Input::setMousePosition(centerX, centerY);
                    continue;
                }

                Input::addMouseDelta(dx, dy);
                Input::setMousePosition(centerX, centerY);

                if (dx != 0.0f || dy != 0.0f)
                {
                    recenterCursor();
                }
            }
            else
            {
                Input::setMousePosition(mouseX, mouseY);
            }
        }
        else if (event.type == ConfigureNotify)
        {
            const bool resized = width != event.xconfigure.width || height != event.xconfigure.height;
            width = event.xconfigure.width;
            height = event.xconfigure.height;
            if (resized)
            {
                recreateBackbuffer();
            }
            if (mouseCaptured)
            {
                recenterCursor();
            }
        }
    }
}

bool X11Window::shouldClose() const
{
    return !running;
}

void X11Window::present(const uint32_t *pixels)
{
    if (!display || !window || !pixels || !image)
        return;

    std::memcpy(imagePixels.data(), pixels, static_cast<size_t>(width) * static_cast<size_t>(height) * sizeof(uint32_t));

    Drawable target = window;
    if (hasDbe && dbeBackBuffer)
    {
        target = dbeBackBuffer;
    }

    XPutImage(
        display,
        target,
        DefaultGC(display, DefaultScreen(display)),
        image,
        0, 0,
        0, 0,
        width,
        height);

    if (hasDbe && dbeBackBuffer)
    {
        XdbeSwapInfo swapInfo;
        swapInfo.swap_window = window;
        swapInfo.swap_action = XdbeUndefined;
        XdbeSwapBuffers(display, &swapInfo, 1);
    }

    XFlush(display);
}
