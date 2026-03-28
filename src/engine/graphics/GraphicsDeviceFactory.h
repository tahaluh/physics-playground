#pragma once

#include <memory>

#include "engine/graphics/GraphicsBackend.h"

class IGraphicsDevice;

namespace GraphicsDeviceFactory
{
std::unique_ptr<IGraphicsDevice> create(GraphicsBackend backend);
}
