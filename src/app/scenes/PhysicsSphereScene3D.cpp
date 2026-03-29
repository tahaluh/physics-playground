#include "app/scenes/PhysicsSphereScene3D.h"

#include <memory>

#include "engine/physics/3d/BallBody3D.h"
#include "engine/physics/3d/BorderSphereBody3D.h"
#include "engine/physics/3d/PhysicsBody3D.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/PhysicsScene3D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
constexpr float kBoundaryRadius = 4.0f;
constexpr float kBallRadius = 0.45f;
}

PhysicsSphereScene3D::~PhysicsSphereScene3D() = default;

std::unique_ptr<PhysicsSphereScene3D> PhysicsSphereScene3D::createDefault()
{
    auto loadedScene = std::make_unique<PhysicsSphereScene3D>();
    loadedScene->physicsScene = std::make_unique<PhysicsScene3D>();
    loadedScene->renderScene = std::make_unique<Scene3D>();

    loadedScene->borderBodyIndex = loadedScene->getPhysicsScene().getBodies().size();
    loadedScene->getPhysicsScene().addBody(std::make_unique<BorderSphereBody3D>(Vector3::zero(), kBoundaryRadius));

    loadedScene->ballBodyIndex = loadedScene->getPhysicsScene().getBodies().size();
    loadedScene->getPhysicsScene().addBody(
        std::make_unique<BallBody3D>(
            kBallRadius,
            Vector3(0.9f, 1.4f, -0.6f),
            Vector3(1.8f, -0.4f, 1.2f)));

    Entity3D border;
    border.name = "BoundarySphere";
    border.mesh = MeshFactory3D::makeSphere(kBoundaryRadius, 10, 16, 0);
    border.material.solid.color = 0xFF9AD1FF;
    border.material.solid.opacity = 0.18f;
    border.material.wireframe.color = 0xFFBFE6FF;
    border.material.wireframe.opacity = 0.55f;
    border.material.renderSolid = true;
    border.material.renderWireframe = true;
    loadedScene->borderEntityIndex = loadedScene->getRenderScene().getEntities().size();
    loadedScene->getRenderScene().createEntity(border);

    Entity3D ball;
    ball.name = "InnerBall";
    ball.mesh = MeshFactory3D::makeSphere(kBallRadius, 8, 12, 0);
    ball.material.solid.color = 0xFFFF9F1C;
    ball.material.solid.opacity = 1.0f;
    ball.material.wireframe.color = 0xFFFFD166;
    ball.material.wireframe.opacity = 1.0f;
    ball.material.renderSolid = true;
    ball.material.renderWireframe = false;
    loadedScene->ballEntityIndex = loadedScene->getRenderScene().getEntities().size();
    loadedScene->getRenderScene().createEntity(ball);

    loadedScene->syncRenderScene();
    return loadedScene;
}

PhysicsScene3D &PhysicsSphereScene3D::getPhysicsScene()
{
    return *physicsScene;
}

const PhysicsScene3D &PhysicsSphereScene3D::getPhysicsScene() const
{
    return *physicsScene;
}

Scene3D &PhysicsSphereScene3D::getRenderScene()
{
    return *renderScene;
}

const Scene3D &PhysicsSphereScene3D::getRenderScene() const
{
    return *renderScene;
}

PhysicsBody3D *PhysicsSphereScene3D::getBallBody()
{
    if (!physicsScene || ballBodyIndex == kInvalidIndex || ballBodyIndex >= physicsScene->getBodies().size())
    {
        return nullptr;
    }

    return physicsScene->getBodies()[ballBodyIndex].get();
}

const PhysicsBody3D *PhysicsSphereScene3D::getBallBody() const
{
    if (!physicsScene || ballBodyIndex == kInvalidIndex || ballBodyIndex >= physicsScene->getBodies().size())
    {
        return nullptr;
    }

    return physicsScene->getBodies()[ballBodyIndex].get();
}

void PhysicsSphereScene3D::setWorldOffset(const Vector3 &offset)
{
    worldOffset = offset;
    syncRenderScene();
}

const Vector3 &PhysicsSphereScene3D::getWorldOffset() const
{
    return worldOffset;
}

void PhysicsSphereScene3D::syncRenderScene()
{
    if (!physicsScene || !renderScene)
    {
        return;
    }

    auto &bodies = physicsScene->getBodies();
    auto &entities = renderScene->getEntities();
    if (borderBodyIndex < bodies.size() && borderEntityIndex < entities.size())
    {
        entities[borderEntityIndex].transform.position = bodies[borderBodyIndex]->getPosition() + worldOffset;
        entities[borderEntityIndex].transform.rotation = Vector3::zero();
        entities[borderEntityIndex].transform.scale = Vector3::one();
    }

    if (ballBodyIndex < bodies.size() && ballEntityIndex < entities.size())
    {
        entities[ballEntityIndex].transform.position = bodies[ballBodyIndex]->getPosition() + worldOffset;
        entities[ballEntityIndex].transform.rotation = Vector3::zero();
        entities[ballEntityIndex].transform.scale = Vector3::one();
    }
}
