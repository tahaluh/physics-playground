#pragma once

class Camera3D;
struct Material3D;
struct Mesh3D;
class Renderer2D;
struct Transform3D;

class WireframeRenderer3D
{
public:
    void drawMesh(Renderer2D &renderer, const Camera3D &camera, const Mesh3D &mesh, const Material3D &material, const Transform3D &transform) const;
};
