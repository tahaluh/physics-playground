#pragma once

class IWindow
{
public:
    virtual bool create(int width, int height, const char *title) = 0;
    virtual void pollEvents() = 0;
    virtual bool shouldClose() const = 0;

    virtual ~IWindow() = default;
};
