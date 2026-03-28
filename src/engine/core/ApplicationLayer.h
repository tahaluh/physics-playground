#pragma once

class Renderer2D;

class ApplicationLayer
{
public:
    virtual ~ApplicationLayer() = default;

    virtual void onAttach(int viewportWidth, int viewportHeight) = 0;
    virtual void onResize(int viewportWidth, int viewportHeight) = 0;
    virtual void onFixedUpdate(float dt) = 0;
    virtual void onRender(Renderer2D &renderer) const = 0;
};
