#pragma once

#include <memory>
#include <string>

#include "engine/math/Transform3D.h"
#include "engine/render/3d/Material3D.h"

class Scene3D;

enum class BodyMotionType3D
{
    Static,
    Dynamic,
    Kinematic,
};

enum class BodyShapeType3D
{
    Cube,
    Sphere,
};

struct BodyObject3DDesc
{
    std::string name;
    BodyMotionType3D motionType = BodyMotionType3D::Static;
    BodyShapeType3D shapeType = BodyShapeType3D::Cube;
    Transform3D transform;
    Material3D material = Material3D{};
    bool renderEnabled = true;
    int sphereRings = 16;
    int sphereSegments = 24;
};

class BodyObject3D
{
public:
    ~BodyObject3D();

    static std::unique_ptr<BodyObject3D> create(const BodyObject3DDesc &desc);

    bool isValid() const;
    void destroy();

    Scene3D &getRenderScene();
    const Scene3D &getRenderScene() const;

    const BodyObject3DDesc &getConfig() const;
    void setTransform(const Transform3D &transform);
    const Transform3D &getTransform() const;
    void syncRenderScene();

private:
    std::unique_ptr<Scene3D> renderScene;
    std::size_t entityIndex = static_cast<std::size_t>(-1);
    BodyObject3DDesc config;
};
