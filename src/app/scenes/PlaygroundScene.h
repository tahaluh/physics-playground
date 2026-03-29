#pragma once

#include <vector>

#include "app/objects/ComposedObject3D.h"
#include "app/objects/RingObject.h"
#include "app/objects/SphereArenaObject.h"
#include "app/objects/SphereObject.h"
#include "engine/math/Vector2.h"
#include "engine/math/Vector3.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/scene/3d/AmbientLight.h"
#include "engine/scene/3d/DirectionalLight.h"
#include "engine/scene/3d/PointLight.h"
#include "engine/scene/3d/SpotLight.h"

struct PlaygroundSceneDesc
{
    AmbientLight ambientLight;
    Vector2 gravity2D = Vector2::zero();
    Vector3 gravity3D = Vector3::zero();
    Vector3 cameraPosition = Vector3::zero();
    Vector3 cameraRotation = Vector3::zero();
    std::vector<RingObjectDesc> ringObjects;
    std::vector<SphereArenaObjectDesc> sphereArenaObjects;
    std::vector<SphereObjectDesc> sphereObjects;
    std::vector<ComposedObject3DDesc> composedObjects;
    std::vector<DirectionalLightDesc> directionalLights;
    std::vector<PointLightDesc> pointLights;
    std::vector<SpotLightDesc> spotLights;
};

PlaygroundSceneDesc makeDefaultPlaygroundSceneDesc();
