#pragma once

#include <memory>
#include <vector>

#include "engine/physics/BroadPhase.h"

class BoxCollider;

class GpuBroadPhase : public BroadPhase
{
public:
    explicit GpuBroadPhase(BroadPhaseCompute *computeBackend = nullptr);

    void setComputeBackend(BroadPhaseCompute *computeBackend);
    std::vector<BroadPhasePair> computePairs(const std::vector<BodyObject *> &bodies) override;

private:
    bool buildBodyInput(const BodyObject &body, BroadPhaseBodyInput &input) const;

    BroadPhaseCompute *computeBackend = nullptr;
    std::unique_ptr<BroadPhase> fallbackBroadPhase;
};
