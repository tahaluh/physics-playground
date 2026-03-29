#include "engine/scene/objects/SphereArenaObject.h"

#include <memory>

#include "engine/physics/3d/BorderSphereBody3D.h"
#include "engine/physics/3d/PhysicsBody3D.h"
#include "engine/physics/3d/PhysicsWorld3D.h"
#include "engine/physics/3d/RigidBody3D.h"
#include "engine/physics/3d/shapes/BoxCollider3D.h"
#include "engine/physics/3d/shapes/SphereCollider3D.h"
#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/PhysicsScene3D.h"
#include "engine/scene/3d/Scene3D.h"

SphereArenaObject::~SphereArenaObject() = default;

SphereArenaObjectDesc SphereArenaObject::makeDefaultDesc()
{
    SphereArenaObjectDesc desc;
    desc.borderMaterial.solid = MaterialPresets3D::makePlastic(0xFF9AD1FF, 0.52f);
    desc.borderMaterial.solid.opacity = 0.32f;
    desc.borderMaterial.solid.diffuseFactor = 0.85f;
    desc.borderMaterial.solid.metallic = 0.0f;
    desc.borderMaterial.solid.doubleSidedLighting = true;
    desc.borderMaterial.wireframe.baseColor = 0xFFBFE6FF;
    desc.borderMaterial.wireframe.opacity = 0.35f;
    desc.borderMaterial.renderSolid = true;
    desc.borderMaterial.renderWireframe = true;

    desc.ballMaterial.solid = MaterialPresets3D::makeBrushedSteel(0xFFFFFFFF);
    desc.ballMaterial.solid.opacity = 1.0f;
    desc.ballMaterial.solid.emissiveColor = 0x00000000;
    desc.ballMaterial.wireframe.baseColor = 0xFF707070;
    desc.ballMaterial.wireframe.opacity = 1.0f;
    desc.ballMaterial.renderSolid = true;
    desc.ballMaterial.renderWireframe = true;

    desc.cubeSize = desc.ballRadius * 2.0f;
    desc.cubeMaterial.solid = MaterialPresets3D::makeRubber(0xFF50C878);
    desc.cubeMaterial.wireframe.baseColor = 0xFFB8FFD4;
    desc.cubeMaterial.wireframe.opacity = 0.75f;
    desc.cubeMaterial.renderSolid = true;
    desc.cubeMaterial.renderWireframe = true;
    return desc;
}

std::unique_ptr<SphereArenaObject> SphereArenaObject::create(const SphereArenaObjectDesc &desc)
{
    auto object = std::make_unique<SphereArenaObject>();
    object->config = desc;
    object->worldOffset = desc.worldOffset;
    object->physicsScene = std::make_unique<PhysicsScene3D>();
    object->renderScene = std::make_unique<Scene3D>();
    object->renderScene->getAmbientLight().color = 0xFFFFFFFF;
    object->renderScene->getAmbientLight().intensity = 0.2f;

    object->borderBodyIndex = object->physicsScene->getBodies().size();
    object->physicsScene->addBody(std::make_unique<BorderSphereBody3D>(Vector3::zero(), desc.boundaryRadius));

    object->ballBodyIndex = object->physicsScene->getBodies().size();
    auto ballBody = std::make_unique<RigidBody3D>(
        std::make_unique<SphereCollider3D>(desc.ballRadius),
        desc.ballStartPosition,
        Vector3::zero(),
        desc.ballStartVelocity,
        1.0f,
        desc.ballSurfaceMaterial,
        desc.ballRigidBodySettings);
    object->physicsScene->addBody(std::move(ballBody));

    if (desc.enableCubeBody)
    {
        object->cubeBodyIndex = object->physicsScene->getBodies().size();
        auto cubeBody = std::make_unique<RigidBody3D>(
            std::make_unique<BoxCollider3D>(Vector3::one() * (desc.cubeSize * 0.5f)),
            desc.cubeStartPosition,
            Vector3::zero(),
            desc.cubeStartVelocity,
            1.0f,
            desc.cubeSurfaceMaterial,
            desc.cubeRigidBodySettings);
        object->physicsScene->addBody(std::move(cubeBody));
    }

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

    if (desc.enableCubeBody)
    {
        Entity3D cube;
        cube.name = "InnerCube";
        cube.mesh = MeshFactory3D::makeCube(desc.cubeSize);
        cube.material = desc.cubeMaterial;
        object->cubeEntityIndex = object->renderScene->getEntities().size();
        object->renderScene->createEntity(cube);
    }

    object->syncRenderScene();
    return object;
}

std::unique_ptr<SphereArenaObject> SphereArenaObject::createDefault()
{
    return create(makeDefaultDesc());
}

void SphereArenaObject::destroy()
{
    physicsScene.reset();
    renderScene.reset();
    borderBodyIndex = kInvalidIndex;
    ballBodyIndex = kInvalidIndex;
    cubeBodyIndex = kInvalidIndex;
    borderEntityIndex = kInvalidIndex;
    ballEntityIndex = kInvalidIndex;
    cubeEntityIndex = kInvalidIndex;
}

bool SphereArenaObject::isValid() const
{
    return static_cast<bool>(physicsScene) && static_cast<bool>(renderScene);
}

PhysicsScene3D &SphereArenaObject::getPhysicsScene() { return *physicsScene; }
const PhysicsScene3D &SphereArenaObject::getPhysicsScene() const { return *physicsScene; }
Scene3D &SphereArenaObject::getRenderScene() { return *renderScene; }
const Scene3D &SphereArenaObject::getRenderScene() const { return *renderScene; }

PhysicsBody3D *SphereArenaObject::getBallBody()
{
    if (!isValid() || ballBodyIndex == kInvalidIndex || ballBodyIndex >= physicsScene->getBodies().size())
        return nullptr;
    return physicsScene->getBodies()[ballBodyIndex].get();
}

const PhysicsBody3D *SphereArenaObject::getBallBody() const
{
    if (!isValid() || ballBodyIndex == kInvalidIndex || ballBodyIndex >= physicsScene->getBodies().size())
        return nullptr;
    return physicsScene->getBodies()[ballBodyIndex].get();
}

PhysicsBody3D *SphereArenaObject::getCubeBody()
{
    if (!isValid() || cubeBodyIndex == kInvalidIndex || cubeBodyIndex >= physicsScene->getBodies().size())
        return nullptr;
    return physicsScene->getBodies()[cubeBodyIndex].get();
}

const PhysicsBody3D *SphereArenaObject::getCubeBody() const
{
    if (!isValid() || cubeBodyIndex == kInvalidIndex || cubeBodyIndex >= physicsScene->getBodies().size())
        return nullptr;
    return physicsScene->getBodies()[cubeBodyIndex].get();
}

void SphereArenaObject::setWorldOffset(const Vector3 &offset)
{
    worldOffset = offset;
    syncRenderScene();
}

const Vector3 &SphereArenaObject::getWorldOffset() const { return worldOffset; }

void SphereArenaObject::step(PhysicsWorld3D &physicsWorld, float dt)
{
    if (!physicsScene)
        return;
    physicsWorld.step(*physicsScene, dt);
    syncRenderScene();
}

void SphereArenaObject::syncRenderScene()
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

    if (cubeBodyIndex < bodies.size() && cubeEntityIndex < entities.size())
    {
        entities[cubeEntityIndex].transform.position = bodies[cubeBodyIndex]->getPosition() + worldOffset;
        entities[cubeEntityIndex].transform.rotation = Vector3::zero();
        entities[cubeEntityIndex].transform.setCustomRotationMatrix(bodies[cubeBodyIndex]->getRotationMatrix());
        entities[cubeEntityIndex].transform.scale = Vector3::one();
    }
}
