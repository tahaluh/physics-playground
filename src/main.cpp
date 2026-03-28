#include <chrono>
#include <thread>
#include <iostream>

#include "platform/linux/X11Window.h"
#include "render/Renderer2D.h"
#include "scene/Scene2D.h"
#include "physics/BallBody2D.h"
#include "physics/BallInvertBody2D.h"
#include "physics/BorderCircleBody2D.h"

int main()
{
    std::cout << "Starting..." << std::endl;

    X11Window window;
    if (!window.create(800, 600, "Phys Playground"))
    {
        return 1;
    }

    Renderer2D renderer(&window);

    Scene2D scene;
    scene.setGravity(Vector2(0.0f, 0.0f));
    scene.addBody(std::make_unique<BorderCircleBody2D>(Vector2(400, 300), 200));
    // Bola com reflexão realista (branca)
    scene.addBody(std::make_unique<BallBody2D>(10, Vector2(420, 270), Vector2(600, 400), 0xFFFFFFFF));
    // Bola com inversão de eixo (vermelha)
    scene.addBody(std::make_unique<BallInvertBody2D>(10, Vector2(380, 330), Vector2(-600, -400), 0xFFCC2222));

    using clock = std::chrono::high_resolution_clock;
    auto last = clock::now();

    const float targetFrameTime = 1.0f / 60.0f; // 60 FPS
    while (!window.shouldClose())
    {
        auto frameStart = clock::now();
        float dt = std::chrono::duration<float>(frameStart - last).count();
        if (dt > 0.033f)
        {
            dt = 0.033f;
        }
        last = frameStart;

        window.pollEvents();

        scene.update(dt);
        scene.checkCollisions();

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
    return 0;
}
