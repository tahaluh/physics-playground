#include "app/scenes/PlaygroundScene.h"

#include <cstdint>

#include "engine/render/3d/mesh/MeshFactory3D.h"

namespace
{
constexpr int kCubeGridSize = 6;
constexpr float kCubeSize = 1.0f;
constexpr float kCubeSpacing = 0.32f;

const Vector3 kGridCenter(0.0f, 3.5f, 0.0f);
const Vector3 kPointLightPosition(10.0f, 16.0f, 12.0f);
const Vector3 kSpotLightPosition(-14.0f, 20.0f, 10.0f);
const Vector3 kSpotLightDirection = Vector3(0.5f, -0.82f, -0.28f).normalized();

float getGridStep()
{
    return kCubeSize + kCubeSpacing;
}

Vector3 getGridOrigin()
{
    const float step = getGridStep();
    return Vector3(
        kGridCenter.x - static_cast<float>(kCubeGridSize - 1) * 0.5f * step,
        kGridCenter.y,
        kGridCenter.z - static_cast<float>(kCubeGridSize - 1) * 0.5f * step);
}

uint32_t makeCubeColor(int column, int row, int layer)
{
    const uint32_t seed =
        static_cast<uint32_t>((column + 1) * 73856093) ^
        static_cast<uint32_t>((row + 1) * 19349663) ^
        static_cast<uint32_t>((layer + 1) * 83492791);
    const uint8_t red = static_cast<uint8_t>(60 + (seed & 0x7F));
    const uint8_t green = static_cast<uint8_t>(60 + ((seed >> 8) & 0x7F));
    const uint8_t blue = static_cast<uint8_t>(60 + ((seed >> 16) & 0x7F));
    return 0xFF000000u |
           (static_cast<uint32_t>(red) << 16) |
           (static_cast<uint32_t>(green) << 8) |
           static_cast<uint32_t>(blue);
}

ComposedObject3DDesc makeCubeDesc(int column, int row, int layer, const Vector3 &position)
{
    ComposedObject3DDesc desc;
    desc.name = "GridCube_" + std::to_string(column) + "_" + std::to_string(row) + "_" + std::to_string(layer);
    desc.worldOffset = Vector3::zero();

    SceneBodyNodeDesc3D node;
    node.name = "Cube";
    node.render.enabled = true;
    node.render.localPosition = position;
    node.render.mesh = MeshFactory3D::makeCube(kCubeSize);
    node.render.material.solid = MaterialPresets3D::makePlastic(makeCubeColor(column, row, layer), 0.3f + 0.06f * static_cast<float>((column + row + layer) % 4));
    node.render.material.wireframe.baseColor = 0xFFFFFFFF;
    node.render.material.wireframe.opacity = 0.16f;
    node.render.material.renderSolid = true;
    node.render.material.renderWireframe = false;
    desc.nodes.push_back(node);
    return desc;
}
}

PlaygroundSceneDesc makeDefaultPlaygroundSceneDesc()
{
    PlaygroundSceneDesc desc;
    desc.ambientLight.color = 0xFFFFFFFF;
    desc.ambientLight.intensity = 0.16f;
    desc.cameraPosition = Vector3(0.0f, 15.0f, 28.0f);
    desc.cameraRotation = Vector3(-0.34f, 0.0f, 0.0f);

    const Vector3 origin = getGridOrigin();
    const float step = getGridStep();
    for (int row = 0; row < kCubeGridSize; ++row)
    {
        for (int column = 0; column < kCubeGridSize; ++column)
        {
            for (int layer = 0; layer < kCubeGridSize; ++layer)
            {
                const Vector3 position(
                    origin.x + static_cast<float>(column) * step,
                    origin.y + static_cast<float>(row) * step,
                    origin.z + static_cast<float>(layer) * step);
                desc.composedObjects.push_back(makeCubeDesc(column, row, layer, position));
            }
        }
    }

    desc.pointLights.push_back({kPointLightPosition, 0xFFFFE3BF, 20.0f, 30.0f});
    desc.spotLights.push_back({kSpotLightPosition, kSpotLightDirection, 0xFFBFD7FF, 14.0f, 34.0f, 0.96f, 0.86f});

    {
        Camera3D lightProbeCamera;
        lightProbeCamera.transform.position = desc.cameraPosition;
        lightProbeCamera.transform.rotation = desc.cameraRotation;
        const Vector3 initialCameraForward = lightProbeCamera.getForward();
        desc.directionalLights.push_back({
            initialCameraForward.lengthSquared() > 0.0f ? initialCameraForward.normalized() : Vector3(0.0f, 0.0f, -1.0f),
            0xFFFFFFFF,
            0.55f});
    }

    return desc;
}
