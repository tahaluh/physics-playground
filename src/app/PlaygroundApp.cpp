#include "app/PlaygroundApp.h"

#include <memory>

#include "app/demos/RenderDemo3D.h"
#include "engine/core/Application.h"
#include "engine/graphics/GraphicsBackend.h"

ApplicationConfig PlaygroundApp::makeConfig() const
{
    ApplicationConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
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
    return application.run(std::make_unique<RenderDemo3D>());
}
