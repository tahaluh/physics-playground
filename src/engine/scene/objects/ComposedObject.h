#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "engine/math/Quaternion.h"
#include "engine/math/Vector3.h"
#include "engine/render/3d/Material.h"
#include "engine/render/3d/mesh/Mesh.h"

class Scene;

struct RenderNodeDesc
{
    bool enabled = true;
    Mesh mesh;
    Material material = Material{};
    Vector3 localPosition = Vector3::zero();
    Quaternion localRotation = Quaternion::identity();
    Vector3 localScale = Vector3::one();
};

struct SceneBodyNodeDesc
{
    std::string name;
    RenderNodeDesc render;
};

struct ComposedObjectDesc
{
    std::string name;
    Vector3 worldOffset = Vector3::zero();
    std::vector<SceneBodyNodeDesc> nodes;
};

class ComposedObject
{
public:
    ~ComposedObject();

    static std::unique_ptr<ComposedObject> create(const ComposedObjectDesc &desc);

    bool isValid() const;
    void destroy();

    Scene &getRenderScene();
    const Scene &getRenderScene() const;

    void setWorldOffset(const Vector3 &offset);
    const Vector3 &getWorldOffset() const;
    void syncRenderScene();

private:
    struct NodeBinding
    {
        std::string name;
        Vector3 localPosition = Vector3::zero();
        Quaternion localRotation = Quaternion::identity();
        Vector3 localScale = Vector3::one();
        bool renderEnabled = false;
        std::size_t entityIndex = kInvalidIndex;
    };

    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::unique_ptr<Scene> renderScene;
    ComposedObjectDesc config;
    Vector3 worldOffset = Vector3::zero();
    std::vector<NodeBinding> bindings;
};
