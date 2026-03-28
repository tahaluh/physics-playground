#include <chrono>
#include <iostream>
#include <thread>

#include "platform/linux/X11Window.h"
#include "physics/BallBody2D.h"
#include "physics/BorderCircleBody2D.h"
#include "physics/PhysicsWorld2D.h"
#include "render/Renderer2D.h"
#include "scene/Scene2D.h"

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
    PhysicsWorld2D physicsWorld;
    physicsWorld.setGravity(Vector2(0.0f, 980.0f));
    scene.addBody(std::make_unique<BorderCircleBody2D>(Vector2(400, 300), 200));
    scene.addBody(std::make_unique<BallBody2D>(10, Vector2(420, 270), Vector2(600, 400), 0xFFFFFFFF));

    using clock = std::chrono::high_resolution_clock;
    auto last = clock::now();

    const float fixedTimeStep = 1.0f / 120.0f;
    const float targetFrameTime = 1.0f / 60.0f; // 60 FPS
    float accumulator = 0.0f;

    while (!window.shouldClose())
    {
        auto frameStart = clock::now();
        float frameDelta = std::chrono::duration<float>(frameStart - last).count();
        if (frameDelta > 0.25f)
        {
            frameDelta = 0.25f;
        }
        last = frameStart;
        accumulator += frameDelta;

        window.pollEvents();

        while (accumulator >= fixedTimeStep)
        {
            physicsWorld.step(scene, fixedTimeStep);
            accumulator -= fixedTimeStep;
        }

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
