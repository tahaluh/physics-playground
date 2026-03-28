#pragma once

#include <cstdint>

class IWindow
{
public:
    virtual bool create(int width, int height, const char *title) = 0;
    virtual void pollEvents() = 0;
    virtual bool shouldClose() const = 0;
    virtual void present(const uint32_t *pixels) = 0;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    virtual ~IWindow() = default;

protected:
    int width = 0;
    int height = 0;
};
