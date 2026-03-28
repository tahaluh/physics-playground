#include "engine/graphics/GraphicsDeviceFactory.h"

#include "engine/graphics/IGraphicsDevice.h"
#include "engine/graphics/software/SoftwareGraphicsDevice.h"
#include "engine/graphics/vulkan/VulkanGraphicsDevice.h"

namespace GraphicsDeviceFactory
{
std::unique_ptr<IGraphicsDevice> create(GraphicsBackend backend)
{
    if (backend == GraphicsBackend::Software)
    {
        return std::make_unique<SoftwareGraphicsDevice>();
    }

    return std::make_unique<VulkanGraphicsDevice>();
}
}
