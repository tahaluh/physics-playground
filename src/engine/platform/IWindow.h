#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

class IWindow
{
public:
    virtual bool create(int width, int height, const char *title) = 0;
    virtual void pollEvents() = 0;
    virtual bool shouldClose() const = 0;
    virtual void present(const uint32_t *pixels) = 0;
    virtual void setMouseCaptured(bool captured) = 0;
    virtual void setTitle(const char *title) = 0;
    virtual const char *const *getRequiredVulkanInstanceExtensions(uint32_t &count) const = 0;
    virtual bool createVulkanSurface(VkInstance instance, VkSurfaceKHR &surface) const = 0;
    virtual void *getNativeDisplayHandle() = 0;
    virtual unsigned long getNativeWindowHandle() = 0;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    virtual ~IWindow() = default;

protected:
    int width = 0;
    int height = 0;
};
