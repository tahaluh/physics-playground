#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include "engine/render/debug/LightDebug3D.h"
#include "engine/render/debug/PhysicsDebug3D.h"
#include "engine/scene/3d/Scene3D.h"
#include "engine/scene/objects/ComposedObject3D.h"
#include "engine/scene/objects/RingObject.h"
#include "engine/scene/objects/SphereArenaObject.h"
#include "engine/scene/objects/SquareObject.h"

namespace DebugSceneComposer3D
{
struct Options
{
    bool showLightDebugMarkers = false;
    bool showPhysicsDebugMarkers = false;
    float physicsDebugSolidOpacity = 0.24f;
    float physicsDebugWireframeOpacity = 0.12f;
};

inline void applyPhysicsDebugTransparency(Scene3D &scene, const Options &options)
{
    for (Entity3D &entity : scene.getEntities())
    {
        if (entity.material.renderSolid)
        {
            entity.material.solid.opacity = std::min(entity.material.solid.opacity, options.physicsDebugSolidOpacity);
        }

        if (entity.material.renderWireframe)
        {
            entity.material.wireframe.opacity = std::min(entity.material.wireframe.opacity, options.physicsDebugWireframeOpacity);
        }
    }
}

inline void appendOverlays(
    Scene3D &targetScene,
    const Options &options,
    const std::vector<std::unique_ptr<RingObject>> &ringObjects,
    const std::vector<std::unique_ptr<SquareObject>> &squareObjects,
    const std::vector<std::size_t> &squareObjectRingIndices,
    const std::vector<std::unique_ptr<SphereArenaObject>> &sphereArenaObjects,
    const std::vector<std::unique_ptr<ComposedObject3D>> &composedObjects)
{
    if (options.showPhysicsDebugMarkers)
    {
        applyPhysicsDebugTransparency(targetScene, options);
    }

    if (options.showLightDebugMarkers)
    {
        LightDebug3D::appendLightMarkers(targetScene, targetScene);
    }

    if (!options.showPhysicsDebugMarkers)
    {
        return;
    }

    for (const auto &ringObject : ringObjects)
    {
        if (ringObject)
        {
            ringObject->appendDebugMarkers(targetScene);
        }
    }

    for (std::size_t i = 0; i < squareObjects.size() && i < squareObjectRingIndices.size(); ++i)
    {
        if (!squareObjects[i])
        {
            continue;
        }

        const std::size_t ringIndex = squareObjectRingIndices[i];
        if (ringIndex >= ringObjects.size() || !ringObjects[ringIndex])
        {
            continue;
        }

        squareObjects[i]->appendDebugMarkers(targetScene, ringObjects[ringIndex]->getConfig());
    }

    for (const auto &sphereArenaObject : sphereArenaObjects)
    {
        if (sphereArenaObject)
        {
            PhysicsDebug3D::appendPhysicsSceneMarkers(
                sphereArenaObject->getPhysicsScene(),
                targetScene,
                sphereArenaObject->getWorldOffset());
        }
    }

    for (const auto &composedObject : composedObjects)
    {
        if (composedObject)
        {
            PhysicsDebug3D::appendPhysicsSceneMarkers(
                composedObject->getPhysicsScene(),
                targetScene,
                composedObject->getWorldOffset());
        }
    }
}
}
