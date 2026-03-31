#include "engine/physics/GpuBroadPhase.h"

#include <utility>

#include "engine/physics/BoxCollider.h"
#include "engine/physics/SapBroadPhase.h"
#include "engine/scene/objects/BodyObject.h"

GpuBroadPhase::GpuBroadPhase(BroadPhaseCompute *computeBackend)
    : computeBackend(computeBackend),
      fallbackBroadPhase(std::make_unique<SapBroadPhase>())
{
}

void GpuBroadPhase::setComputeBackend(BroadPhaseCompute *newComputeBackend)
{
    computeBackend = newComputeBackend;
}

bool GpuBroadPhase::buildBodyInput(const BodyObject &body, BroadPhaseBodyInput &input) const
{
    const Collider *collider = body.getCollider();
    const auto *boxCollider = dynamic_cast<const BoxCollider *>(collider);
    if (!boxCollider)
    {
        return false;
    }

    const Transform &transform = body.getTransform();
    input.shapeType = static_cast<uint32_t>(BroadPhaseShapeType::Box);
    input.position = transform.position;
    input.rotation = transform.rotation;
    input.scale = transform.scale;
    input.offset = boxCollider->offset;
    input.halfExtents = boxCollider->halfExtents;
    return true;
}

std::vector<BroadPhasePair> GpuBroadPhase::computePairs(const std::vector<BodyObject *> &bodies)
{
    if (!computeBackend)
    {
        return fallbackBroadPhase->computePairs(bodies);
    }

    std::vector<BodyObject *> validBodies;
    std::vector<BroadPhaseBodyInput> inputs;
    validBodies.reserve(bodies.size());
    inputs.reserve(bodies.size());
    for (BodyObject *body : bodies)
    {
        if (!body)
        {
            continue;
        }

        BroadPhaseBodyInput input;
        if (!buildBodyInput(*body, input))
        {
            return fallbackBroadPhase->computePairs(bodies);
        }

        validBodies.push_back(body);
        inputs.push_back(input);
    }

    std::vector<BroadPhaseCandidatePair> candidatePairs;
    if (!computeBackend->computeCandidatePairs(inputs, candidatePairs))
    {
        return fallbackBroadPhase->computePairs(bodies);
    }

    std::vector<BroadPhasePair> pairs;
    pairs.reserve(candidatePairs.size());
    for (const BroadPhaseCandidatePair &candidatePair : candidatePairs)
    {
        if (candidatePair.aIndex >= validBodies.size() || candidatePair.bIndex >= validBodies.size())
        {
            continue;
        }

        BodyObject *bodyA = validBodies[candidatePair.aIndex];
        BodyObject *bodyB = validBodies[candidatePair.bIndex];
        if (!bodyA || !bodyB || bodyA == bodyB)
        {
            continue;
        }

        pairs.push_back({bodyA, bodyB});
    }

    return pairs;
}
