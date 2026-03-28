#include <chrono>
#include <thread>
#include <iostream>

#include "platform/linux/X11Window.h"
#include "render/Renderer2D.h"
#include "scene/Scene2D.h"
#include "physics/PhysicsBody2D.h"
#include "physics/BorderBoxBody2D.h"
#include "physics/BallBody2D.h"
#include "physics/shapes/CircleShape.h"
#include "physics/shapes/RectShape.h"

int main()
{
    std::cout << "Starting..." << std::endl;

    IWindow *window = new X11Window();
    window->create(800, 600, "Phys Playground");

    Scene2D scene;
    scene.addBody(std::make_unique<BorderCircleBody2D>(Vector2(400, 300), 200));
    scene.addBody(std::make_unique<BallBody2D>(
        std::make_unique<CircleShape>(10), Vector2(420, 270), Vector2(600, 400)));

    using clock = std::chrono::high_resolution_clock;
    auto last = clock::now();

    const float targetFrameTime = 1.0f / 60.0f; // 60 FPS
    while (!window->shouldClose())
    {
        auto frameStart = clock::now();
        float dt = std::chrono::duration<float>(frameStart - last).count();
        last = frameStart;

        window->pollEvents();

        scene.update(dt);
        scene.checkCollisions();

        Renderer2D renderer(window);
        renderer.clear(0xFF000000);
        scene.render(renderer);
        renderer.present();

        // Controle de FPS
        auto frameEnd = clock::now();
        float frameTime = std::chrono::duration<float>(frameEnd - frameStart).count();
        if (frameTime < targetFrameTime)
        {
            auto sleepTime = std::chrono::duration<float>(targetFrameTime - frameTime);
            std::this_thread::sleep_for(sleepTime);
        }
    }

    delete window;
    return 0;
}