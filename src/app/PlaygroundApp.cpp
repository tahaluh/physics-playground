#include "app/PlaygroundApp.h"

#include <chrono>
#include <thread>

#include "app/demos/RenderDemo3D.h"
#include "engine/platform/linux/X11Window.h"
#include "engine/render/2d/Renderer2D.h"

namespace
{
    constexpr int kWindowWidth = 800;
    constexpr int kWindowHeight = 600;
    constexpr float kFixedTimeStep = 1.0f / 120.0f;
    constexpr float kTargetFrameTime = 1.0f / 60.0f;
    constexpr float kMaxFrameDelta = 0.25f;
    constexpr uint32_t kBackgroundColor = 0xFF000000;
}

int PlaygroundApp::run()
{
    X11Window window;
    if (!window.create(kWindowWidth, kWindowHeight, "Phys Playground"))
    {
        return 1;
    }

    Renderer2D renderer(&window);
    RenderDemo3D renderDemo;
    renderDemo.initialize(window.getWidth(), window.getHeight());

    using clock = std::chrono::high_resolution_clock;
    auto last = clock::now();

    float accumulator = 0.0f;

    while (!window.shouldClose())
    {
        auto frameStart = clock::now();
        float frameDelta = std::chrono::duration<float>(frameStart - last).count();
        if (frameDelta > kMaxFrameDelta)
        {
            frameDelta = kMaxFrameDelta;
        }
        last = frameStart;
        accumulator += frameDelta;

        window.pollEvents();

        if (renderer.getWidth() != window.getWidth() || renderer.getHeight() != window.getHeight())
        {
            renderer.resize(window.getWidth(), window.getHeight());
            renderDemo.resizeViewport(window.getWidth(), window.getHeight());
        }

        while (accumulator >= kFixedTimeStep)
        {
            renderDemo.step(kFixedTimeStep);
            accumulator -= kFixedTimeStep;
        }

        renderer.clear(kBackgroundColor);
        renderer.clearDepth();
        renderDemo.render(renderer);
        renderer.present();

        auto frameEnd = clock::now();
        float frameTime = std::chrono::duration<float>(frameEnd - frameStart).count();
        if (frameTime < kTargetFrameTime)
        {
            auto sleepTime = std::chrono::duration<float>(kTargetFrameTime - frameTime);
            std::this_thread::sleep_for(sleepTime);
        }
    }

    return 0;
}
