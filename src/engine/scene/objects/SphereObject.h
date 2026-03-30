#pragma once

#include <cstddef>
#include <memory>

#include "engine/math/Vector3.h"
#include "engine/render/3d/Material.h"

class Scene;

struct SphereObjectDesc
{
    Vector3 worldOffset = Vector3::zero();
    float radius = 1.0f;
    int sphereRings = 16;
    int sphereSegments = 24;
    Material material = Material{};
};

class SphereObject
{
public:
    ~SphereObject();

    static SphereObjectDesc makeDefaultDesc();
    static std::unique_ptr<SphereObject> create(const SphereObjectDesc &desc);
    static std::unique_ptr<SphereObject> createDefault();
    void destroy();
    bool isValid() const;

    Scene &getRenderScene();
    const Scene &getRenderScene() const;
    void setWorldOffset(const Vector3 &offset);
    const Vector3 &getWorldOffset() const;
    void syncRenderScene();

private:
    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::unique_ptr<Scene> renderScene;
    std::size_t sphereEntityIndex = kInvalidIndex;
    Vector3 worldOffset = Vector3::zero();
    SphereObjectDesc config;
};
