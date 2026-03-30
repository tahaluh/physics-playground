#pragma once

#include <vector>

#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/scene/3d/AmbientLight.h"
#include "engine/scene/3d/DirectionalLight.h"
#include "engine/scene/objects/BodyObject3D.h"
#include "engine/scene/3d/PointLight.h"
#include "engine/scene/3d/SpotLight.h"

struct PlaygroundSceneDesc
{
    AmbientLight ambientLight;
    Vector3 cameraPosition = Vector3::zero();
    Vector3 cameraRotation = Vector3::zero();
    std::vector<BodyObject3DDesc> bodies;
    std::vector<DirectionalLightDesc> directionalLights;
    std::vector<PointLightDesc> pointLights;
    std::vector<SpotLightDesc> spotLights;
};

PlaygroundSceneDesc makeDefaultPlaygroundSceneDesc();
