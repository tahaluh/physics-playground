#include "app/scenes/PlaygroundScene.h"

#include "engine/render/3d/mesh/MeshFactory3D.h"

namespace
{
    const Vector3 kRingWorldOffset(-5.0f, 0.0f, 0.0f);
    const Vector3 kSphereWorldOffset(5.0f, 0.0f, 0.0f);
    const Vector3 kPointLightPosition = kSphereWorldOffset + Vector3(2.8f, 0.5f, 0.0f);
    const Vector3 kSpotLightPosition = kSphereWorldOffset + Vector3(-5.2f, 2.4f, 2.0f);
    const Vector3 kSpotLightDirection = (kSphereWorldOffset - kSpotLightPosition).normalized();
    const float kSpotLightIntensity = 1.1f;
    const float kSpotLightRange = 10.0f;
    const float kSpotLightInnerConeCos = 0.96f;
    const float kSpotLightOuterConeCos = 0.86f;
    const uint32_t kSpotLightColor = 0xFFFFFFFF;
    const Vector3 kShadowTestObjectOffset = kSphereWorldOffset + Vector3(1.6f, 0.2f, 5.0f);

    RingObjectDesc makeRingObjectDesc()
    {
        RingObjectDesc desc;
        desc.worldOffset = kRingWorldOffset;
        desc.center = Vector2(400.0f, 300.0f);
        desc.ballStartPosition = Vector2(230.0f, 180.0f);
        desc.ballStartVelocity = Vector2(110.0f, -40.0f);
        desc.simulationScale = 100.0f;
        desc.borderRadiusPixels = 200.0f;
        desc.borderThicknessPixels = 12.0f;
        desc.ballRadiusPixels = 10.0f;
        desc.ballOutlineThicknessWorld = 0.02f;
        desc.planeThicknessWorld = 0.04f;
        desc.planeZ = 0.0f;
        desc.borderSegments = 96;
        desc.ballSegments = 48;
        desc.borderColor = 0xFFFFFFFF;
        desc.ballColor = 0xFFFFFFFF;
        desc.physicsSurfaceMaterial = PhysicsSurfaceMaterial2D{0.55f, 0.68f, 0.6f};
        desc.rigidBodySettings = RigidBodySettings2D{0.025f, 0.03f, true};
        desc.borderMaterial.solid.baseColor = desc.borderColor;
        desc.borderMaterial.wireframe.baseColor = desc.borderColor;
        desc.borderMaterial.renderSolid = true;
        desc.borderMaterial.renderWireframe = false;
        desc.ballMaterial.solid.baseColor = desc.ballColor;
        desc.ballMaterial.wireframe.baseColor = desc.ballColor;
        desc.ballMaterial.renderSolid = true;
        desc.ballMaterial.renderWireframe = false;
        return desc;
    }

    SquareObjectDesc makeSquareObjectDesc()
    {
        SquareObjectDesc desc;
        desc.ringObjectIndex = 0;
        desc.startPosition = Vector2(400.0f, 100.0f);
        desc.startVelocity = Vector2::zero();
        desc.sizePixels = 44.0f;
        desc.color = 0xFFFF8A5B;
        desc.physicsSurfaceMaterial = PhysicsSurfaceMaterial2D{0.55f, 0.68f, 0.6f};
        desc.rigidBodySettings = RigidBodySettings2D{0.025f, 0.03f, true};
        desc.material.solid.baseColor = desc.color;
        desc.material.wireframe.baseColor = desc.color;
        desc.material.renderSolid = true;
        desc.material.renderWireframe = false;
        return desc;
    }

    SphereArenaObjectDesc makeSphereArenaObjectDesc()
    {
        SphereArenaObjectDesc desc;
        desc.worldOffset = kSphereWorldOffset;
        desc.boundaryRadius = 4.0f;
        desc.ballRadius = 0.45f;
        desc.ballStartPosition = Vector3(0.9f, 1.4f, -0.6f);
        desc.ballStartVelocity = Vector3(1.8f, -0.4f, 1.2f);
        desc.ballSurfaceMaterial = PhysicsSurfaceMaterial3D{0.55f, 0.68f, 0.6f};
        desc.ballRigidBodySettings = RigidBodySettings3D{0.025f, 0.03f, true, Vector3::zero()};
        desc.boundarySphereRings = 10;
        desc.boundarySphereSegments = 16;
        desc.ballSphereRings = 16;
        desc.ballSphereSegments = 24;
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

        desc.enableCubeBody = true;
        desc.cubeSize = desc.ballRadius * 2.0f;
        desc.cubeStartPosition = Vector3(-1.15f, 0.4f, 0.8f);
        desc.cubeStartVelocity = Vector3(-1.4f, 0.25f, -1.1f);
        desc.cubeSurfaceMaterial = PhysicsSurfaceMaterial3D{0.55f, 0.68f, 0.6f};
        desc.cubeRigidBodySettings = RigidBodySettings3D{0.1f, 0.55f, true, Vector3(0.0f, -0.08f, 0.0f)};
        desc.cubeMaterial.solid = MaterialPresets3D::makeRubber(0xFF50C878);
        desc.cubeMaterial.wireframe.baseColor = 0xFFB8FFD4;
        desc.cubeMaterial.wireframe.opacity = 0.75f;
        desc.cubeMaterial.renderSolid = true;
        desc.cubeMaterial.renderWireframe = true;
        return desc;
    }

    ComposedObject3DDesc makeShadowTestObjectDesc()
    {
        ComposedObject3DDesc desc;
        desc.name = "ShadowTestObject";
        desc.worldOffset = Vector3::zero();

        SceneBodyNodeDesc3D node;
        node.name = "ShadowTestSphere";
        node.render.enabled = true;
        node.render.localPosition = kShadowTestObjectOffset;
        node.render.mesh = MeshFactory3D::makeSphere(1.2f, 14, 20, 0);
        node.render.material.solid = MaterialPresets3D::makePlastic(0xFF2F6BFF, 0.4f);
        node.render.material.wireframe.baseColor = 0xFF7EA2FF;
        node.render.material.renderSolid = true;
        node.render.material.renderWireframe = false;
        node.physics.enabled = false;
        node.collider.enabled = false;

        desc.nodes.push_back(node);
        return desc;
    }
}

PlaygroundSceneDesc makeDefaultPlaygroundSceneDesc()
{
    PlaygroundSceneDesc desc;
    desc.ambientLight.color = 0xFFFFFFFF;
    desc.ambientLight.intensity = 0.2f;
    desc.gravity2D = Vector2(0.0f, 980.0f);
    desc.gravity3D = Vector3(0.0f, -7.5f, 0.0f);
    desc.cameraPosition = Vector3(0.0f, 3.5f, 22.0f);
    desc.cameraRotation = Vector3(-0.12f, 0.0f, 0.0f);

    desc.ringObjects.push_back(makeRingObjectDesc());
    desc.squareObjects.push_back(makeSquareObjectDesc());
    desc.sphereArenaObjects.push_back(makeSphereArenaObjectDesc());
    desc.composedObjects.push_back(makeShadowTestObjectDesc());

    Camera3D lightProbeCamera;
    lightProbeCamera.transform.position = desc.cameraPosition;
    lightProbeCamera.transform.rotation = desc.cameraRotation;
    const Vector3 initialCameraForward = lightProbeCamera.getForward();
    desc.directionalLights.push_back({initialCameraForward.lengthSquared() > 0.0f ? initialCameraForward.normalized() : Vector3(0.0f, 0.0f, -1.0f),
                                      0xFFFFFFFF,
                                      0.9f});
    desc.pointLights.push_back({kPointLightPosition,
                                0xFFFF4040,
                                0.75f,
                                8.0f});
    desc.spotLights.push_back({kSpotLightPosition,
                               kSpotLightDirection,
                               kSpotLightColor,
                               kSpotLightIntensity,
                               kSpotLightRange,
                               kSpotLightInnerConeCos,
                               kSpotLightOuterConeCos});
    return desc;
}
