#include "engine/graphics/software/SoftwareGraphicsDevice.h"

#include "engine/render/2d/Renderer2D.h"
#include "engine/render/3d/SceneRenderer3D.h"

SoftwareGraphicsDevice::~SoftwareGraphicsDevice() = default;

GraphicsBackend SoftwareGraphicsDevice::getBackend() const
{
    return GraphicsBackend::Software;
}

const char *SoftwareGraphicsDevice::getBackendName() const
{
    return "Software";
}

void SoftwareGraphicsDevice::configurePresentation(bool, int)
{
}

bool SoftwareGraphicsDevice::initialize(IWindow &windowRef)
{
    window = &windowRef;
    renderer = std::make_unique<Renderer2D>(&windowRef);
    sceneRenderer = std::make_unique<SceneRenderer3D>();
    return true;
}

void SoftwareGraphicsDevice::resize(int width, int height)
{
    if (renderer)
    {
        renderer->resize(width, height);
    }
}

int SoftwareGraphicsDevice::getWidth() const
{
    return renderer ? renderer->getWidth() : 0;
}

int SoftwareGraphicsDevice::getHeight() const
{
    return renderer ? renderer->getHeight() : 0;
}

void SoftwareGraphicsDevice::beginFrame(uint32_t clearColor)
{
    if (!renderer)
    {
        return;
    }

    renderer->clear(clearColor);
    renderer->clearDepth();
}

void SoftwareGraphicsDevice::renderScene3D(const Camera3D &camera, const Scene3D &scene)
{
    if (renderer && sceneRenderer)
    {
        sceneRenderer->render(*renderer, camera, scene);
    }
}

void SoftwareGraphicsDevice::endFrame()
{
}

void SoftwareGraphicsDevice::present()
{
    if (renderer)
    {
        renderer->present();
    }
}

Renderer2D *SoftwareGraphicsDevice::getRenderer() const
{
    return renderer.get();
}
