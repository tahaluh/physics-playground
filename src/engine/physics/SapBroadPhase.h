#pragma once

#include <vector>

#include "engine/physics/BroadPhase.h"

class SapBroadPhase : public BroadPhase
{
public:
    std::vector<BroadPhasePair> computePairs(const std::vector<BodyObject *> &bodies) override;

private:
    std::vector<BodyObject *> sweepAndPruneOrder;
};
