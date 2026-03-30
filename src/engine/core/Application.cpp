#include "engine/core/Application.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include "engine/core/ApplicationLayer.h"
#include "engine/graphics/GraphicsDeviceFactory.h"
#include "engine/graphics/IGraphicsDevice.h"
#include "engine/input/Input.h"
#include "engine/platform/IWindow.h"
#include "engine/platform/PlatformFactory.h"

namespace
{
std::atomic<bool> gInterruptRequested = false;

struct ProcessMetrics
{
    float cpuPercent = 0.0f;
    float ramMegabytes = 0.0f;
};

void handleInterrupt(int)
{
    gInterruptRequested.store(true);
}

long long readProcessCpuTicks()
{
#if defined(__linux__)
    std::ifstream statFile("/proc/self/stat");
    if (!statFile)
    {
        return -1;
    }

    std::string statLine;
    std::getline(statFile, statLine);
    const std::size_t closingParen = statLine.rfind(')');
    if (closingParen == std::string::npos || closingParen + 2 >= statLine.size())
    {
        return -1;
    }

    std::istringstream fields(statLine.substr(closingParen + 2));
    std::string field;
    long long utime = 0;
    long long stime = 0;
    for (int index = 3; fields >> field; ++index)
    {
        if (index == 14)
        {
            utime = std::atoll(field.c_str());
        }
        else if (index == 15)
        {
            stime = std::atoll(field.c_str());
            break;
        }
    }

    return utime + stime;
#else
    return -1;
#endif
}

float readProcessRamMegabytes()
{
#if defined(__linux__)
    std::ifstream statmFile("/proc/self/statm");
    if (!statmFile)
    {
        return 0.0f;
    }

    long long totalPages = 0;
    long long residentPages = 0;
    statmFile >> totalPages >> residentPages;
    if (residentPages <= 0)
    {
        return 0.0f;
    }

    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0)
    {
        return 0.0f;
    }

    return static_cast<float>(residentPages * pageSize) / (1024.0f * 1024.0f);
#else
    return 0.0f;
#endif
}

ProcessMetrics sampleProcessMetrics(long long &previousCpuTicks, const std::chrono::high_resolution_clock::time_point &previousSampleTime)
{
    ProcessMetrics metrics;
#if defined(__linux__)
    const long long currentCpuTicks = readProcessCpuTicks();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float elapsedSeconds = std::chrono::duration<float>(currentTime - previousSampleTime).count();
    const long clockTicksPerSecond = sysconf(_SC_CLK_TCK);

    if (previousCpuTicks >= 0 && currentCpuTicks >= 0 && elapsedSeconds > 0.0f && clockTicksPerSecond > 0)
    {
        const float cpuSeconds =
            static_cast<float>(currentCpuTicks - previousCpuTicks) / static_cast<float>(clockTicksPerSecond);
        metrics.cpuPercent = (cpuSeconds / elapsedSeconds) * 100.0f;
    }

    previousCpuTicks = currentCpuTicks;
    metrics.ramMegabytes = readProcessRamMegabytes();
#else
    (void)previousCpuTicks;
    (void)previousSampleTime;
#endif
    return metrics;
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

    std::unique_ptr<IWindow> window = PlatformFactory::createWindow();
    if (!window || !window->create(config.windowWidth, config.windowHeight, config.title))
    {
        std::signal(SIGINT, previousSigIntHandler);
        return 1;
    }
    window->setMouseCaptured(true);

    std::unique_ptr<IGraphicsDevice> graphicsDevice = GraphicsDeviceFactory::create(config.preferredGraphicsBackend);
    if (!graphicsDevice)
    {
        std::signal(SIGINT, previousSigIntHandler);
        return 1;
    }

    graphicsDevice->configurePresentation(config.vsyncEnabled, config.targetFrameRate);

    if (!graphicsDevice->initialize(*window))
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
    window->setTitle(initialTitleBuffer);

    layer->onAttach(window->getWidth(), window->getHeight());
    bool mouseCaptured = true;

    using clock = std::chrono::high_resolution_clock;
    const bool frameLimiterEnabled = config.targetFrameRate > 0;
    const auto targetFrameDuration = frameLimiterEnabled
        ? std::chrono::duration<float>(1.0f / static_cast<float>(config.targetFrameRate))
        : std::chrono::duration<float>(0.0f);
    auto last = clock::now();
    auto previousMetricsSampleTime = last;
    long long previousCpuTicks = readProcessCpuTicks();
    float accumulator = 0.0f;
    float titleUpdateAccumulator = 0.0f;
    int framesSinceTitleUpdate = 0;

    while (!window->shouldClose() && !gInterruptRequested.load())
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
        window->pollEvents();

        if (Input::wasKeyPressed(EngineKeyCode::Escape))
        {
            mouseCaptured = false;
            window->setMouseCaptured(false);
        }
        else if (Input::wasKeyPressed(EngineKeyCode::Enter))
        {
            mouseCaptured = true;
            window->setMouseCaptured(true);
        }

        const bool ctrlDown = Input::isKeyDown(EngineKeyCode::ControlLeft) || Input::isKeyDown(EngineKeyCode::ControlRight);
        if (ctrlDown && Input::wasKeyPressed(EngineKeyCode::C))
        {
            gInterruptRequested.store(true);
        }

        if (graphicsDevice->getWidth() != window->getWidth() || graphicsDevice->getHeight() != window->getHeight())
        {
            graphicsDevice->resize(window->getWidth(), window->getHeight());
            layer->onResize(window->getWidth(), window->getHeight());
        }

        layer->onUpdate(frameDelta);

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
            const ProcessMetrics metrics = sampleProcessMetrics(previousCpuTicks, previousMetricsSampleTime);
            previousMetricsSampleTime = clock::now();
            char titleBuffer[256];
            std::snprintf(
                titleBuffer,
                sizeof(titleBuffer),
                "%s | %s | %s | %d target | %.1f FPS | %.2f ms | %s | CPU %.1f%% | RAM %.1f MB",
                config.title,
                graphicsDevice->getBackendName(),
                config.vsyncEnabled ? "VSync On" : "VSync Off",
                config.targetFrameRate,
                fps,
                averageFrameTime * 1000.0f,
                graphicsDevice->getDeviceName(),
                metrics.cpuPercent,
                metrics.ramMegabytes);
            window->setTitle(titleBuffer);

            titleUpdateAccumulator = 0.0f;
            framesSinceTitleUpdate = 0;
        }
    }

    std::signal(SIGINT, previousSigIntHandler);
    return 0;
}
