#include "engine/scene/objects/SphereObject.h"

#include <memory>

#include "engine/render/3d/mesh/MeshFactory3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/PhysicsScene3D.h"
#include "engine/scene/3d/Scene3D.h"

SphereObject::~SphereObject() = default;

SphereObjectDesc SphereObject::makeDefaultDesc()
{
    SphereObjectDesc desc;
    desc.material.solid = MaterialPresets3D::makePlastic(0xFF2F6BFF, 0.4f);
    desc.material.wireframe.baseColor = 0xFF7EA2FF;
    desc.material.renderSolid = true;
    desc.material.renderWireframe = false;
    return desc;
}

std::unique_ptr<SphereObject> SphereObject::create(const SphereObjectDesc &desc)
{
    auto object = std::make_unique<SphereObject>();
    object->config = desc;
    object->worldOffset = desc.worldOffset;
    object->physicsScene = std::make_unique<PhysicsScene3D>();
    object->renderScene = std::make_unique<Scene3D>();

    Entity3D sphere;
    sphere.name = "Sphere";
    sphere.mesh = MeshFactory3D::makeSphere(desc.radius, desc.sphereRings, desc.sphereSegments, 0);
    sphere.material = desc.material;
    object->sphereEntityIndex = object->renderScene->getEntities().size();
    object->renderScene->createEntity(sphere);

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
    sphereEntityIndex = kInvalidIndex;
}

bool SphereObject::isValid() const
{
    return static_cast<bool>(physicsScene) && static_cast<bool>(renderScene);
}

PhysicsScene3D &SphereObject::getPhysicsScene() { return *physicsScene; }
const PhysicsScene3D &SphereObject::getPhysicsScene() const { return *physicsScene; }
Scene3D &SphereObject::getRenderScene() { return *renderScene; }
const Scene3D &SphereObject::getRenderScene() const { return *renderScene; }

void SphereObject::setWorldOffset(const Vector3 &offset)
{
    worldOffset = offset;
    syncRenderScene();
}

const Vector3 &SphereObject::getWorldOffset() const { return worldOffset; }

void SphereObject::syncRenderScene()
{
    if (!isValid())
        return;

    auto &entities = renderScene->getEntities();
    if (sphereEntityIndex < entities.size())
    {
        entities[sphereEntityIndex].transform.position = worldOffset;
        entities[sphereEntityIndex].transform.rotation = Vector3::zero();
        entities[sphereEntityIndex].transform.clearCustomRotationMatrix();
        entities[sphereEntityIndex].transform.scale = Vector3::one();
    }
}
