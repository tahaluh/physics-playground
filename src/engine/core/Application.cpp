#include "engine/core/Application.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <csignal>
#include <thread>

#include "engine/core/ApplicationLayer.h"
#include "engine/graphics/GraphicsDeviceFactory.h"
#include "engine/graphics/IGraphicsDevice.h"
#include "engine/input/Input.h"
#include "engine/platform/linux/X11Window.h"

namespace
{
std::atomic<bool> gInterruptRequested = false;

void handleInterrupt(int)
{
    gInterruptRequested.store(true);
}
}

Application::Application(ApplicationConfig config)
    : config(config)
{
}

int Application::run(std::unique_ptr<ApplicationLayer> layer)
{
    gInterruptRequested.store(false);
    const auto previousSigIntHandler = std::signal(SIGINT, handleInterrupt);

    if (!layer)
    {
        std::signal(SIGINT, previousSigIntHandler);
        return 1;
    }

    X11Window window;
    if (!window.create(config.windowWidth, config.windowHeight, config.title))
    {
        std::signal(SIGINT, previousSigIntHandler);
        return 1;
    }
    window.setMouseCaptured(true);

    std::unique_ptr<IGraphicsDevice> graphicsDevice = GraphicsDeviceFactory::create(config.preferredGraphicsBackend);
    if (!graphicsDevice)
    {
        std::signal(SIGINT, previousSigIntHandler);
        return 1;
    }

    graphicsDevice->configurePresentation(config.vsyncEnabled, config.targetFrameRate);

    if (!graphicsDevice->initialize(window))
    {
        std::signal(SIGINT, previousSigIntHandler);
        return 1;
    }

    char initialTitleBuffer[256];
    std::snprintf(
        initialTitleBuffer,
        sizeof(initialTitleBuffer),
        "%s | %s",
        config.title,
        graphicsDevice->getBackendName());
    window.setTitle(initialTitleBuffer);

    layer->onAttach(window.getWidth(), window.getHeight());
    bool mouseCaptured = true;

    using clock = std::chrono::high_resolution_clock;
    const bool frameLimiterEnabled = config.targetFrameRate > 0;
    const auto targetFrameDuration = frameLimiterEnabled
        ? std::chrono::duration<float>(1.0f / static_cast<float>(config.targetFrameRate))
        : std::chrono::duration<float>(0.0f);
    auto last = clock::now();
    float accumulator = 0.0f;
    float titleUpdateAccumulator = 0.0f;
    int framesSinceTitleUpdate = 0;

    while (!window.shouldClose() && !gInterruptRequested.load())
    {
        auto frameStart = clock::now();
        float frameDelta = std::chrono::duration<float>(frameStart - last).count();
        if (frameDelta > config.maxFrameDelta)
        {
            frameDelta = config.maxFrameDelta;
        }
        last = frameStart;
        accumulator += frameDelta;
        titleUpdateAccumulator += frameDelta;
        ++framesSinceTitleUpdate;

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

        const bool ctrlDown = Input::isKeyDown(EngineKeyCode::ControlLeft) || Input::isKeyDown(EngineKeyCode::ControlRight);
        if (ctrlDown && Input::wasKeyPressed(EngineKeyCode::C))
        {
            gInterruptRequested.store(true);
        }

        if (graphicsDevice->getWidth() != window.getWidth() || graphicsDevice->getHeight() != window.getHeight())
        {
            graphicsDevice->resize(window.getWidth(), window.getHeight());
            layer->onResize(window.getWidth(), window.getHeight());
        }

        while (accumulator >= config.fixedTimeStep)
        {
            layer->onFixedUpdate(config.fixedTimeStep);
            accumulator -= config.fixedTimeStep;
        }

        graphicsDevice->beginFrame(config.clearColor);
        layer->onRender(*graphicsDevice);
        graphicsDevice->endFrame();
        graphicsDevice->present();

        if (frameLimiterEnabled)
        {
            const auto frameEnd = clock::now();
            const auto elapsedFrameTime = frameEnd - frameStart;
            if (elapsedFrameTime < targetFrameDuration)
            {
                std::this_thread::sleep_for(targetFrameDuration - elapsedFrameTime);
            }
        }

        if (titleUpdateAccumulator >= 0.25f)
        {
            const float averageFrameTime = titleUpdateAccumulator / static_cast<float>(framesSinceTitleUpdate);
            const float fps = static_cast<float>(framesSinceTitleUpdate) / titleUpdateAccumulator;
            char titleBuffer[256];
            std::snprintf(
                titleBuffer,
                sizeof(titleBuffer),
                "%s | %s | %s | %d target | %.1f FPS | %.2f ms",
                config.title,
                graphicsDevice->getBackendName(),
                config.vsyncEnabled ? "VSync On" : "VSync Off",
                config.targetFrameRate,
                fps,
                averageFrameTime * 1000.0f);
            window.setTitle(titleBuffer);

            titleUpdateAccumulator = 0.0f;
            framesSinceTitleUpdate = 0;
        }
    }

    std::signal(SIGINT, previousSigIntHandler);
    return 0;
}
