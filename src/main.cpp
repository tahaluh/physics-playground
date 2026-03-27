#include <iostream>

#include "platform/IWindow.h"
#include "platform/linux/X11Window.h"

int main()
{
    std::cout << "Starting..." << std::endl;

    IWindow *window = new X11Window();
    window->create(800, 600, "Phys Playground");

    while (!window->shouldClose())
    {
        window->pollEvents();
    }

    delete window;
    return 0;
}