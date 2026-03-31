#pragma once

#include <cstdint>
#include <vector>

#include "engine/physics/Collider.h"
#include "engine/math/Quaternion.h"

class BodyObject;

struct BroadPhaseEntry
{
    BodyObject *body = nullptr;
    const Collider *collider = nullptr;
    Collider::BroadPhaseBounds bounds;
};

struct BroadPhasePair
{
    BodyObject *a = nullptr;
    BodyObject *b = nullptr;
};

enum class BroadPhaseShapeType : uint32_t
{
    None = 0,
    Box = 1,
};

struct BroadPhaseBodyInput
{
    uint32_t shapeType = static_cast<uint32_t>(BroadPhaseShapeType::None);
    float padding0 = 0.0f;
    float padding1 = 0.0f;
    float padding2 = 0.0f;
    Vector3 position = Vector3::zero();
    float padding3 = 0.0f;
    Quaternion rotation = Quaternion::identity();
    Vector3 scale = Vector3::one();
    float padding4 = 0.0f;
    Vector3 offset = Vector3::zero();
    float padding5 = 0.0f;
    Vector3 halfExtents = Vector3::zero();
    float padding6 = 0.0f;
};

struct BroadPhaseCandidatePair
{
    uint32_t aIndex = 0;
    uint32_t bIndex = 0;
};

class BroadPhaseCompute
{
public:
    virtual ~BroadPhaseCompute() = default;

    virtual bool computeCandidatePairs(
        const std::vector<BroadPhaseBodyInput> &inputs,
        std::vector<BroadPhaseCandidatePair> &pairs) = 0;
};

class BroadPhase
{
public:
    virtual ~BroadPhase() = default;

    virtual std::vector<BroadPhasePair> computePairs(const std::vector<BodyObject *> &bodies) = 0;
};
