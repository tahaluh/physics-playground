#include "app/objects/SphereObject.h"

#include <memory>

#include "engine/physics/3d/BallBody3D.h"
#include "engine/physics/3d/BorderSphereBody3D.h"
#include "engine/physics/3d/PhysicsBody3D.h"
#include "engine/physics/3d/PhysicsWorld3D.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/PhysicsScene3D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
}

SphereObject::~SphereObject() = default;

SphereObjectDesc SphereObject::makeDefaultDesc()
{
    SphereObjectDesc desc;
    desc.borderMaterial.solid.color = 0xFF9AD1FF;
    desc.borderMaterial.solid.opacity = 0.18f;
    desc.borderMaterial.wireframe.color = 0xFFBFE6FF;
    desc.borderMaterial.wireframe.opacity = 0.55f;
    desc.borderMaterial.renderSolid = true;
    desc.borderMaterial.renderWireframe = true;
    desc.borderMaterial.solid.specularStrength = 0.12f;
    desc.borderMaterial.solid.shininess = 24.0f;
    desc.ballMaterial.solid.color = 0xFFFF9F1C;
    desc.ballMaterial.solid.opacity = 1.0f;
    desc.ballMaterial.solid.emissiveColor = 0x00000000;
    desc.ballMaterial.solid.specularStrength = 0.85f;
    desc.ballMaterial.solid.shininess = 64.0f;
    desc.ballMaterial.wireframe.color = 0xFFFFD166;
    desc.ballMaterial.wireframe.opacity = 1.0f;
    desc.ballMaterial.renderSolid = true;
    desc.ballMaterial.renderWireframe = true;
    return desc;
}

std::unique_ptr<SphereObject> SphereObject::create(const SphereObjectDesc &desc)
{
    auto object = std::make_unique<SphereObject>();
    object->config = desc;
    object->worldOffset = desc.worldOffset;
    object->physicsScene = std::make_unique<PhysicsScene3D>();
    object->renderScene = std::make_unique<Scene3D>();
    object->renderScene->getAmbientLight().color = 0xFFFFFFFF;
    object->renderScene->getAmbientLight().intensity = 0.2f;

    object->borderBodyIndex = object->physicsScene->getBodies().size();
    object->physicsScene->addBody(std::make_unique<BorderSphereBody3D>(Vector3::zero(), desc.boundaryRadius));

    object->ballBodyIndex = object->physicsScene->getBodies().size();
    auto ballBody = std::make_unique<BallBody3D>(desc.ballRadius, desc.ballStartPosition, desc.ballStartVelocity);
    ballBody->getMaterial() = desc.physicsMaterial;
    object->physicsScene->addBody(std::move(ballBody));

    Entity3D border;
    border.name = "BoundarySphere";
    border.mesh = MeshFactory3D::makeSphere(desc.boundaryRadius, desc.boundarySphereRings, desc.boundarySphereSegments, 0);
    border.material = desc.borderMaterial;
    object->borderEntityIndex = object->renderScene->getEntities().size();
    object->renderScene->createEntity(border);

    Entity3D ball;
    ball.name = "InnerBall";
    ball.mesh = MeshFactory3D::makeSphere(desc.ballRadius, desc.ballSphereRings, desc.ballSphereSegments, 0);
    ball.material = desc.ballMaterial;
    object->ballEntityIndex = object->renderScene->getEntities().size();
    object->renderScene->createEntity(ball);

    object->syncRenderScene();
    return object;
}

std::unique_ptr<SphereObject> SphereObject::createDefault()
{
    return create(makeDefaultDesc());
}

void SphereObject::destroy()
{
    physicsScene.reset();
    renderScene.reset();
    borderBodyIndex = kInvalidIndex;
    ballBodyIndex = kInvalidIndex;
    borderEntityIndex = kInvalidIndex;
    ballEntityIndex = kInvalidIndex;
}

bool SphereObject::isValid() const
{
    return static_cast<bool>(physicsScene) && static_cast<bool>(renderScene);
}

PhysicsScene3D &SphereObject::getPhysicsScene() { return *physicsScene; }
const PhysicsScene3D &SphereObject::getPhysicsScene() const { return *physicsScene; }
Scene3D &SphereObject::getRenderScene() { return *renderScene; }
const Scene3D &SphereObject::getRenderScene() const { return *renderScene; }

PhysicsBody3D *SphereObject::getBallBody()
{
    if (!isValid() || ballBodyIndex == kInvalidIndex || ballBodyIndex >= physicsScene->getBodies().size())
        return nullptr;
    return physicsScene->getBodies()[ballBodyIndex].get();
}

const PhysicsBody3D *SphereObject::getBallBody() const
{
    if (!isValid() || ballBodyIndex == kInvalidIndex || ballBodyIndex >= physicsScene->getBodies().size())
        return nullptr;
    return physicsScene->getBodies()[ballBodyIndex].get();
}

void SphereObject::setWorldOffset(const Vector3 &offset)
{
    worldOffset = offset;
    syncRenderScene();
}

const Vector3 &SphereObject::getWorldOffset() const { return worldOffset; }

void SphereObject::step(PhysicsWorld3D &physicsWorld, float dt)
{
    if (!physicsScene)
        return;
    physicsWorld.step(*physicsScene, dt);
    syncRenderScene();
}

void SphereObject::syncRenderScene()
{
    if (!isValid())
        return;

    auto &bodies = physicsScene->getBodies();
    auto &entities = renderScene->getEntities();

    if (borderBodyIndex < bodies.size() && borderEntityIndex < entities.size())
    {
        entities[borderEntityIndex].transform.position = bodies[borderBodyIndex]->getPosition() + worldOffset;
        entities[borderEntityIndex].transform.rotation = bodies[borderBodyIndex]->getRotation();
        entities[borderEntityIndex].transform.clearCustomRotationMatrix();
        entities[borderEntityIndex].transform.scale = Vector3::one();
    }

    if (ballBodyIndex < bodies.size() && ballEntityIndex < entities.size())
    {
        entities[ballEntityIndex].transform.position = bodies[ballBodyIndex]->getPosition() + worldOffset;
        entities[ballEntityIndex].transform.rotation = Vector3::zero();
        entities[ballEntityIndex].transform.setCustomRotationMatrix(bodies[ballBodyIndex]->getRotationMatrix());
        entities[ballEntityIndex].transform.scale = Vector3::one();
    }
}
