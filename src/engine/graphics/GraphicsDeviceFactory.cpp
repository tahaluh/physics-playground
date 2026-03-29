#include "engine/graphics/GraphicsDeviceFactory.h"

#include "engine/graphics/IGraphicsDevice.h"
#include "engine/graphics/vulkan/VulkanGraphicsDevice.h"

namespace GraphicsDeviceFactory
{
std::unique_ptr<IGraphicsDevice> create(GraphicsBackend backend)
{
    return std::make_unique<VulkanGraphicsDevice>();
}
}
