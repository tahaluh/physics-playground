#include "app/demos/PhysicsDemo2D.h"

#include <memory>

#include "engine/graphics/IGraphicsDevice.h"
#include "engine/graphics/software/SoftwareGraphicsDevice.h"
#include "engine/physics/2d/BallBody2D.h"
#include "engine/physics/2d/BorderCircleBody2D.h"
#include "engine/physics/2d/PhysicsWorld2D.h"
#include "engine/render/2d/Renderer2D.h"
#include "engine/scene/2d/Scene2D.h"

PhysicsDemo2D::~PhysicsDemo2D() = default;

void PhysicsDemo2D::onAttach(int, int)
{
    scene = std::make_unique<Scene2D>();
    physicsWorld = std::make_unique<PhysicsWorld2D>();

    physicsWorld->setGravity(Vector2(0.0f, 980.0f));
    scene->addBody(std::make_unique<BorderCircleBody2D>(Vector2(400, 300), 200));
    scene->addBody(std::make_unique<BallBody2D>(10, Vector2(230, 270), Vector2(0, 0), 0xFFFFFFFF));
}

void PhysicsDemo2D::onResize(int, int)
{
}

void PhysicsDemo2D::onFixedUpdate(float dt)
{
    if (physicsWorld && scene)
    {
        physicsWorld->step(*scene, dt);
    }
}

void PhysicsDemo2D::onRender(IGraphicsDevice &graphicsDevice) const
{
    auto *softwareDevice = dynamic_cast<SoftwareGraphicsDevice *>(&graphicsDevice);
    if (!softwareDevice || !scene)
    {
        return;
    }

    Renderer2D *renderer = softwareDevice->getRenderer();
    if (!renderer)
    {
        return;
    }

    scene->render(*renderer);
}
