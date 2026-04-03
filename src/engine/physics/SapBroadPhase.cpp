#include "engine/physics/SapBroadPhase.h"

#include <algorithm>

#include "engine/scene/objects/BodyObject.h"

std::vector<BroadPhasePair> SapBroadPhase::computePairs(const std::vector<BodyObject *> &bodies)
{
    const auto overlapsOnYZ = [](const Collider::BroadPhaseBounds &a, const Collider::BroadPhaseBounds &b)
    {
        return a.min.y <= b.max.y &&
               a.max.y >= b.min.y &&
               a.min.z <= b.max.z &&
               a.max.z >= b.min.z;
    };

    if (sweepAndPruneOrder.size() != bodies.size())
    {
        sweepAndPruneOrder = bodies;
    }

    std::vector<BroadPhaseEntry> entries;
    entries.reserve(bodies.size());
    for (BodyObject *body : sweepAndPruneOrder)
    {
        if (!body)
        {
            continue;
        }

        const Collider *collider = body->getCollider();
        if (!collider)
        {
            continue;
        }

        const Collider::BroadPhaseBounds bounds = collider->getBroadPhaseBounds(body->getTransform());
        if (!bounds.valid)
        {
            continue;
        }

        entries.push_back({body, collider, bounds});
    }

    for (std::size_t index = 1; index < entries.size(); ++index)
    {
        BroadPhaseEntry entry = entries[index];
        std::size_t insertIndex = index;
        while (insertIndex > 0 && entries[insertIndex - 1].bounds.min.x > entry.bounds.min.x)
        {
            entries[insertIndex] = entries[insertIndex - 1];
            --insertIndex;
        }
        entries[insertIndex] = entry;
    }

    sweepAndPruneOrder.clear();
    sweepAndPruneOrder.reserve(entries.size());
    for (const BroadPhaseEntry &entry : entries)
    {
        sweepAndPruneOrder.push_back(entry.body);
    }

    std::vector<BroadPhasePair> pairs;
    pairs.reserve(entries.size());

    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        const BroadPhaseEntry &entryA = entries[i];
        for (std::size_t j = i + 1; j < entries.size(); ++j)
        {
            const BroadPhaseEntry &entryB = entries[j];
            if (entryB.bounds.min.x > entryA.bounds.max.x)
            {
                break;
            }

            if (!overlapsOnYZ(entryA.bounds, entryB.bounds))
            {
                continue;
            }

            pairs.push_back({entryA.body, entryB.body});
        }
    }

    return pairs;
}
