#pragma once

#include <cstdint>

#include "engine/graphics/GraphicsBackend.h"

class Camera;
class BroadPhaseCompute;
class IWindow;
class Scene;

class IGraphicsDevice
{
public:
    virtual ~IGraphicsDevice() = default;

    virtual GraphicsBackend getBackend() const = 0;
    virtual const char *getBackendName() const = 0;
    virtual const char *getDeviceName() const = 0;

    virtual void configurePresentation(bool vsyncEnabled, int targetFrameRate) = 0;
    virtual void setLightDebugOverlayEnabled(bool enabled) = 0;
    virtual void setWireframeOverlayEnabled(bool enabled) = 0;
    virtual BroadPhaseCompute *getBroadPhaseCompute() = 0;
    virtual bool initialize(IWindow &window) = 0;
    virtual void resize(int width, int height) = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    virtual void beginFrame(uint32_t clearColor) = 0;
    virtual void renderScene(const Camera &camera, const Scene &scene) = 0;
    virtual void endFrame() = 0;
    virtual void present() = 0;
};
