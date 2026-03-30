#pragma once

#include <vector>

#include "engine/math/Quaternion.h"
#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"
#include "engine/render/3d/Camera.h"
#include "engine/scene/3d/AmbientLight.h"
#include "engine/scene/3d/DirectionalLight.h"
#include "engine/scene/objects/BodyObject.h"
#include "engine/scene/3d/PointLight.h"
#include "engine/scene/3d/SpotLight.h"

struct PlaygroundSceneDesc
{
    AmbientLight ambientLight;
    Vector3 cameraPosition = Vector3::zero();
    Quaternion cameraRotation = Quaternion::identity();
    std::vector<BodyObjectDesc> bodies;
    std::vector<DirectionalLightDesc> directionalLights;
    std::vector<PointLightDesc> pointLights;
    std::vector<SpotLightDesc> spotLights;
};

PlaygroundSceneDesc makeDefaultPlaygroundSceneDesc();
