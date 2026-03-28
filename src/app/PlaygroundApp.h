#pragma once

#include "engine/core/Application.h"

class PlaygroundApp
{
public:
    int run();

private:
    ApplicationConfig makeConfig() const;
};
