#include "app/demos/PhysicsDemo2D.h"

#include "engine/physics/2d/BallBody2D.h"
#include "engine/physics/2d/BorderCircleBody2D.h"
#include "engine/physics/2d/PhysicsWorld2D.h"
#include "engine/render/2d/Renderer2D.h"
#include "engine/scene/2d/Scene2D.h"

void PhysicsDemo2D::initialize()
{
    if (!scene)
        scene = new Scene2D();
    if (!physicsWorld)
        physicsWorld = new PhysicsWorld2D();

    physicsWorld->setGravity(Vector2(0.0f, 980.0f));
    scene->addBody(std::make_unique<BorderCircleBody2D>(Vector2(400, 300), 200));
    scene->addBody(std::make_unique<BallBody2D>(10, Vector2(230, 270), Vector2(0, 0), 0xFFFFFFFF));
}

void PhysicsDemo2D::step(float dt)
{
    physicsWorld->step(*scene, dt);
}

void PhysicsDemo2D::render(Renderer2D &renderer) const
{
    scene->render(renderer);
}
