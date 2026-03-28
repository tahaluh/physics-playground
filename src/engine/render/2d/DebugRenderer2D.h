#pragma once

#include <vector>

struct Manifold2D;

class Renderer2D;

class DebugRenderer2D
{
public:
    void drawContacts(Renderer2D &renderer, const std::vector<Manifold2D> &manifolds) const;
};
