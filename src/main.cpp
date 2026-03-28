#include <iostream>

#include "platform/linux/X11Window.h"
#include "render/Renderer2D.h"

int main()
{
    std::cout << "Starting..." << std::endl;

    IWindow *window = new X11Window();
    window->create(800, 600, "Phys Playground");

    while (!window->shouldClose())
    {
        window->pollEvents();

        Renderer2D renderer(window);

        renderer.clear(0xFFFF0000);

        renderer.fillRect(400, 300, 100, 100, 0xFFFFFFFF);
        renderer.drawRect(400, 300, 100, 100, 0xFF000000);

        renderer.present();
    }

    delete window;
    return 0;
}