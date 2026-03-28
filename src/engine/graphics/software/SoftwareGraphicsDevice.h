#pragma once

#include <memory>

#include "engine/graphics/IGraphicsDevice.h"
#include "engine/render/2d/Renderer2D.h"
#include "engine/render/3d/SceneRenderer3D.h"

class SoftwareGraphicsDevice : public IGraphicsDevice
{
public:
    ~SoftwareGraphicsDevice() override;

    GraphicsBackend getBackend() const override;
    const char *getBackendName() const override;

    void configurePresentation(bool vsyncEnabled, int targetFrameRate) override;
    bool initialize(IWindow &window) override;
    void resize(int width, int height) override;
    int getWidth() const override;
    int getHeight() const override;

    void beginFrame(uint32_t clearColor) override;
    void renderScene3D(const Camera3D &camera, const Scene3D &scene) override;
    void endFrame() override;
    void present() override;

    Renderer2D *getRenderer() const;

private:
    IWindow *window = nullptr;
    std::unique_ptr<Renderer2D> renderer;
    std::unique_ptr<SceneRenderer3D> sceneRenderer;
};
