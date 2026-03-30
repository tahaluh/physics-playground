#include "app/scenes/PlaygroundScene.h"

#include <cstdint>
#include <string>

namespace
{
constexpr int kCubeGridSize = 25;
constexpr float kCubeSize = 1.0f;
constexpr float kCubeSpacing = 0.32f;

const Vector3 kGridCenter(0.0f, 3.5f, 0.0f);

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

BodyObject3DDesc makeCubeDesc(int column, int row, int layer, const Vector3 &position)
{
    BodyObject3DDesc desc;
    desc.name = "GridCube_" + std::to_string(column) + "_" + std::to_string(row) + "_" + std::to_string(layer);
    desc.motionType = BodyMotionType3D::Static;
    desc.shapeType = BodyShapeType3D::Cube;
    desc.transform.position = position;
    desc.transform.scale = Vector3::one() * kCubeSize;
    desc.material.solid = MaterialPresets3D::makePlastic(makeCubeColor(column, row, layer), 0.3f + 0.06f * static_cast<float>((column + row + layer) % 4));
    desc.material.wireframe.baseColor = 0xFFFFFFFF;
    desc.material.wireframe.opacity = 0.16f;
    desc.material.renderSolid = true;
    desc.material.renderWireframe = false;
    return desc;
}
}

PlaygroundSceneDesc makeDefaultPlaygroundSceneDesc()
{
    PlaygroundSceneDesc desc;
    desc.ambientLight.color = 0xFFFFFFFF;
    desc.ambientLight.intensity = 0.22f;
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
                desc.bodies.push_back(makeCubeDesc(column, row, layer, position));
            }
        }
    }

    desc.directionalLights.push_back({
        Vector3(-1.0f, -1.2f, -1.0f).normalized(),
        0xFFFFFFFF,
        0.72f,
        true});

    return desc;
}
