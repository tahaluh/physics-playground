#pragma once

#include <cstdint>
#include <memory>

#include "engine/graphics/GraphicsBackend.h"

class ApplicationLayer;

struct ApplicationConfig
{
    int windowWidth = 800;
    int windowHeight = 600;
    const char *title = "Engine Playground";
    float fixedTimeStep = 1.0f / 120.0f;
    int targetFrameRate = 60;
    bool vsyncEnabled = true;
    float maxFrameDelta = 0.25f;
    int maxFixedStepsPerFrame = 4;
    uint32_t clearColor = 0xFF000000;
    GraphicsBackend preferredGraphicsBackend = GraphicsBackend::Vulkan;
};

class Application
{
public:
    explicit Application(ApplicationConfig config = {});

    int run(std::unique_ptr<ApplicationLayer> layer);

private:
    ApplicationConfig config;
};
