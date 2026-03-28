#include "engine/core/Application.h"

#include <chrono>
#include <thread>

#include "engine/core/ApplicationLayer.h"
#include "engine/input/Input.h"
#include "engine/platform/linux/X11Window.h"
#include "engine/render/2d/Renderer2D.h"

Application::Application(ApplicationConfig config)
    : config(config)
{
}

int Application::run(std::unique_ptr<ApplicationLayer> layer)
{
    if (!layer)
    {
        return 1;
    }

    X11Window window;
    if (!window.create(config.windowWidth, config.windowHeight, config.title))
    {
        return 1;
    }
    window.setMouseCaptured(true);

    Renderer2D renderer(&window);
    layer->onAttach(window.getWidth(), window.getHeight());
    bool mouseCaptured = true;

    using clock = std::chrono::high_resolution_clock;
    auto last = clock::now();
    float accumulator = 0.0f;

    while (!window.shouldClose())
    {
        auto frameStart = clock::now();
        float frameDelta = std::chrono::duration<float>(frameStart - last).count();
        if (frameDelta > config.maxFrameDelta)
        {
            frameDelta = config.maxFrameDelta;
        }
        last = frameStart;
        accumulator += frameDelta;

        Input::beginFrame();
        window.pollEvents();

        if (Input::wasKeyPressed(EngineKeyCode::Escape))
        {
            mouseCaptured = false;
            window.setMouseCaptured(false);
        }
        else if (Input::wasKeyPressed(EngineKeyCode::Enter))
        {
            mouseCaptured = true;
            window.setMouseCaptured(true);
        }

        if (renderer.getWidth() != window.getWidth() || renderer.getHeight() != window.getHeight())
        {
            renderer.resize(window.getWidth(), window.getHeight());
            layer->onResize(window.getWidth(), window.getHeight());
        }

        while (accumulator >= config.fixedTimeStep)
        {
            layer->onFixedUpdate(config.fixedTimeStep);
            accumulator -= config.fixedTimeStep;
        }

        renderer.clear(config.clearColor);
        renderer.clearDepth();
        layer->onRender(renderer);
        renderer.present();

        auto frameEnd = clock::now();
        float frameTime = std::chrono::duration<float>(frameEnd - frameStart).count();
        if (frameTime < config.targetFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::duration<float>(config.targetFrameTime - frameTime));
        }
    }

    return 0;
}
