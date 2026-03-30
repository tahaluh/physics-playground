#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "engine/math/Vector3.h"
#include "engine/render/3d/Material3D.h"
#include "engine/render/3d/mesh/Mesh3D.h"

class Scene3D;

struct RenderNodeDesc3D
{
    bool enabled = true;
    Mesh3D mesh;
    Material3D material = Material3D{};
    Vector3 localPosition = Vector3::zero();
    Vector3 localRotation = Vector3::zero();
    Vector3 localScale = Vector3::one();
};

struct SceneBodyNodeDesc3D
{
    std::string name;
    RenderNodeDesc3D render;
};

struct ComposedObject3DDesc
{
    std::string name;
    Vector3 worldOffset = Vector3::zero();
    std::vector<SceneBodyNodeDesc3D> nodes;
};

class ComposedObject3D
{
public:
    ~ComposedObject3D();

    static std::unique_ptr<ComposedObject3D> create(const ComposedObject3DDesc &desc);

    bool isValid() const;
    void destroy();

    Scene3D &getRenderScene();
    const Scene3D &getRenderScene() const;

    void setWorldOffset(const Vector3 &offset);
    const Vector3 &getWorldOffset() const;
    void syncRenderScene();

private:
    struct NodeBinding
    {
        std::string name;
        Vector3 localPosition = Vector3::zero();
        Vector3 localRotation = Vector3::zero();
        Vector3 localScale = Vector3::one();
        bool renderEnabled = false;
        std::size_t entityIndex = kInvalidIndex;
    };

    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::unique_ptr<Scene3D> renderScene;
    ComposedObject3DDesc config;
    Vector3 worldOffset = Vector3::zero();
    std::vector<NodeBinding> bindings;
};
