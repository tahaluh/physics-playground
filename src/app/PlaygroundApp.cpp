#include "app/PlaygroundApp.h"

#include <memory>

#include "app/demos/Demo.h"
#include "engine/core/Application.h"
#include "engine/graphics/GraphicsBackend.h"

ApplicationConfig PlaygroundApp::makeConfig() const
{
    ApplicationConfig config;
    config.windowWidth = 0;
    config.windowHeight = 0;
    config.title = "Phys Playground";
    config.clearColor = 0xFF000000;
    config.targetFrameRate = 60;
    config.vsyncEnabled = true;
    config.preferredGraphicsBackend = GraphicsBackend::Vulkan;
    return config;
}

int PlaygroundApp::run()
{
    Application application(makeConfig());
    return application.run(std::make_unique<Demo>());
}
