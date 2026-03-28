#pragma once

class Renderer2D;

class PhysicsDemo2D
{
public:
    void initialize();
    void step(float dt);
    void render(Renderer2D &renderer) const;

private:
    class Scene2D *scene = nullptr;
    class PhysicsWorld2D *physicsWorld = nullptr;
};
