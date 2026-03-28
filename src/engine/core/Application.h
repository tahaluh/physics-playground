#pragma once

#include <cstdint>
#include <memory>

class ApplicationLayer;

struct ApplicationConfig
{
    int windowWidth = 800;
    int windowHeight = 600;
    const char *title = "Engine Playground";
    float fixedTimeStep = 1.0f / 120.0f;
    float targetFrameTime = 1.0f / 60.0f;
    float maxFrameDelta = 0.25f;
    uint32_t clearColor = 0xFF000000;
};

class Application
{
public:
    explicit Application(ApplicationConfig config = {});

    int run(std::unique_ptr<ApplicationLayer> layer);

private:
    ApplicationConfig config;
};
