#include "app/scenes/PlaygroundScene.h"

#include <cmath>
#include <cstdint>
#include <string>

namespace
{
constexpr int kCubeGridSize = 10;
constexpr float kCubeSize = 1.0f;
constexpr float kCubeNonTouchMargin = 0.1f;
constexpr float kCubeSpacing = (std::sqrt(3.0f) - kCubeSize) + kCubeNonTouchMargin;
constexpr float kTau = 6.28318530718f;
constexpr float kGridFrontDistance = 8.0f;
constexpr float kMaxAngularSpeed = 1.0f;

float getGridStep()
{
    return kCubeSize + kCubeSpacing;
}

Vector3 getGridOrigin()
{
    const float step = getGridStep();
    const float halfSpan = static_cast<float>(kCubeGridSize - 1) * 0.5f * step;
    return Vector3(
        -halfSpan,
        -halfSpan,
        -kGridFrontDistance - static_cast<float>(kCubeGridSize - 1) * step);
}

uint32_t makeCubeSeed(int column, int row, int layer)
{
    return static_cast<uint32_t>((column + 1) * 73856093) ^
           static_cast<uint32_t>((row + 1) * 19349663) ^
           static_cast<uint32_t>((layer + 1) * 83492791);
}

float hashToAngleRadians(uint32_t value)
{
    return (static_cast<float>(value) / 4294967295.0f) * kTau;
}

Quaternion makeCubeRotation(int column, int row, int layer)
{
    uint32_t value = makeCubeSeed(column, row, layer);
    value = value * 1664525u + 1013904223u;
    const float rotationX = hashToAngleRadians(value);
    value = value * 1664525u + 1013904223u;
    const float rotationY = hashToAngleRadians(value);
    value = value * 1664525u + 1013904223u;
    const float rotationZ = hashToAngleRadians(value);
    return Quaternion::fromEulerXYZ(Vector3(rotationX, rotationY, rotationZ));
}

float hashToUnitFloat(uint32_t value)
{
    return static_cast<float>(value) / 4294967295.0f;
}

Vector3 makeCubeAngularVelocity(int column, int row, int layer)
{
    uint32_t value = makeCubeSeed(column, row, layer);
    value = value * 22695477u + 1u;
    const float angularVelocityX = hashToUnitFloat(value) * kMaxAngularSpeed;
    value = value * 22695477u + 1u;
    const float angularVelocityY = hashToUnitFloat(value) * kMaxAngularSpeed;
    value = value * 22695477u + 1u;
    const float angularVelocityZ = hashToUnitFloat(value) * kMaxAngularSpeed;
    return Vector3(angularVelocityX, angularVelocityY, angularVelocityZ);
}

uint32_t makeCubeColor(int column, int row, int layer)
{
    const uint32_t seed = makeCubeSeed(column, row, layer);
    const uint8_t red = static_cast<uint8_t>(60 + (seed & 0x7F));
    const uint8_t green = static_cast<uint8_t>(60 + ((seed >> 8) & 0x7F));
    const uint8_t blue = static_cast<uint8_t>(60 + ((seed >> 16) & 0x7F));
    return 0xFF000000u |
           (static_cast<uint32_t>(red) << 16) |
           (static_cast<uint32_t>(green) << 8) |
           static_cast<uint32_t>(blue);
}

BodyObjectDesc makeCubeDesc(int column, int row, int layer, const Vector3 &position)
{
    BodyObjectDesc desc;
    desc.name = "GridCube_" + std::to_string(column) + "_" + std::to_string(row) + "_" + std::to_string(layer);
    desc.motionType = BodyMotionType::Dynamic;
    desc.simulateOnGpu = true;
    desc.shapeType = BodyShapeType::Cube;
    desc.transform.position = position;
    desc.transform.rotation = makeCubeRotation(column, row, layer);
    desc.transform.scale = Vector3::one() * kCubeSize;
    desc.physics.angularVelocity = makeCubeAngularVelocity(column, row, layer);
    desc.material.solid = MaterialPresets::makePlastic(makeCubeColor(column, row, layer), 0.3f + 0.06f * static_cast<float>((column + row + layer) % 4));
    desc.material.wireframe.baseColor = 0xFFFFFFFF;
    desc.material.wireframe.opacity = 1.0f;
    desc.material.wireframe.unlit = true;
    desc.material.wireframe.ambientFactor = 0.0f;
    desc.material.wireframe.diffuseFactor = 0.0f;
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
    desc.cameraPosition = Vector3::zero();
    desc.cameraRotation = Quaternion::identity();

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
