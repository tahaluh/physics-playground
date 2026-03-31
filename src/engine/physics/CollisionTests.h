#pragma once

#include <vector>

#include "engine/physics/CollisionPoints.h"

class BoxCollider;
class PlaneCollider;
struct Transform;

namespace CollisionTests
{
CollisionPoints testBoxBox(
    const BoxCollider &boxA,
    const Transform &transformA,
    const BoxCollider &boxB,
    const Transform &transformB);

CollisionPoints testBoxPlane(
    const BoxCollider &box,
    const Transform &boxTransform,
    const PlaneCollider &plane,
    const Transform &planeTransform);

std::vector<CollisionPoints> testBoxPlaneContactManifold(
    const BoxCollider &box,
    const Transform &boxTransform,
    const PlaneCollider &plane,
    const Transform &planeTransform);

CollisionPoints invert(const CollisionPoints &collision);
}
