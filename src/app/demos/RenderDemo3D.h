#pragma once

class Renderer2D;

class RenderDemo3D
{
public:
    void initialize(int viewportWidth, int viewportHeight);
    void resizeViewport(int viewportWidth, int viewportHeight);
    void step(float dt);
    void render(Renderer2D &renderer) const;

private:
    class Camera3D *camera = nullptr;
    class SolidRenderer3D *solidRenderer = nullptr;
    class WireframeRenderer3D *wireframeRenderer = nullptr;
    class Transform3D *cubeTransform = nullptr;
    float cubeRotation = 0.0f;
};
