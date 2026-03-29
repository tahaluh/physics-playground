#include "engine/graphics/vulkan/VulkanGraphicsDevice.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>

#include "engine/math/Matrix4.h"
#include "engine/platform/IWindow.h"
#include "engine/render/3d/Camera3D.h"
#include "engine/scene/3d/Entity3D.h"
#include "engine/scene/3d/Scene3D.h"

namespace
{
struct QueueFamilySelection
{
    uint32_t graphicsFamily = UINT32_MAX;
    uint32_t presentFamily = UINT32_MAX;

    bool isComplete() const
    {
        return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX;
    }
};

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

QueueFamilySelection findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    QueueFamilySelection selection;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            selection.graphicsFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport)
        {
            selection.presentFamily = i;
        }

        if (selection.isComplete())
        {
            break;
        }
    }

    return selection;
}

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount > 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount > 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool deviceSupportsRequiredExtensions(VkPhysicalDevice physicalDevice)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

    bool hasSwapchain = false;
    for (const auto &extension : extensions)
    {
        if (std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
        {
            hasSwapchain = true;
            break;
        }
    }

    return hasSwapchain;
}

std::vector<char> readFirstExistingBinary(const std::initializer_list<const char *> &paths)
{
    for (const char *path : paths)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            continue;
        }

        const std::streamsize fileSize = file.tellg();
        if (fileSize <= 0)
        {
            continue;
        }

        std::vector<char> buffer(static_cast<size_t>(fileSize));
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        if (file.good() || file.eof())
        {
            return buffer;
        }
    }

    return {};
}

uint32_t toVulkanColor(uint32_t argb)
{
    const uint32_t a = (argb >> 24) & 0xFF;
    const uint32_t r = (argb >> 16) & 0xFF;
    const uint32_t g = (argb >> 8) & 0xFF;
    const uint32_t b = argb & 0xFF;
    return (a << 24) | (b << 16) | (g << 8) | r;
}

std::array<float, 4> colorToFloat4(uint32_t argb)
{
    const float a = static_cast<float>((argb >> 24) & 0xFF) / 255.0f;
    const float r = static_cast<float>((argb >> 16) & 0xFF) / 255.0f;
    const float g = static_cast<float>((argb >> 8) & 0xFF) / 255.0f;
    const float b = static_cast<float>(argb & 0xFF) / 255.0f;
    return {r, g, b, a};
}

std::array<float, 4> applyAmbientLighting(
    uint32_t baseColor,
    uint32_t emissiveColor,
    float ambientFactor,
    uint32_t ambientLightColor,
    float ambientLightIntensity)
{
    const std::array<float, 4> base = colorToFloat4(baseColor);
    const std::array<float, 4> emissive = colorToFloat4(emissiveColor);
    const std::array<float, 4> ambient = colorToFloat4(ambientLightColor);
    const float clampedAmbientFactor = std::clamp(ambientFactor, 0.0f, 1.0f);
    const float clampedAmbientIntensity = std::max(ambientLightIntensity, 0.0f);

    std::array<float, 4> result{};
    result[0] = std::clamp(base[0] * ambient[0] * clampedAmbientFactor * clampedAmbientIntensity + emissive[0], 0.0f, 1.0f);
    result[1] = std::clamp(base[1] * ambient[1] * clampedAmbientFactor * clampedAmbientIntensity + emissive[1], 0.0f, 1.0f);
    result[2] = std::clamp(base[2] * ambient[2] * clampedAmbientFactor * clampedAmbientIntensity + emissive[2], 0.0f, 1.0f);
    result[3] = base[3];
    return result;
}
}

VulkanGraphicsDevice::~VulkanGraphicsDevice()
{
    destroyDevice();
}

GraphicsBackend VulkanGraphicsDevice::getBackend() const
{
    return GraphicsBackend::Vulkan;
}

const char *VulkanGraphicsDevice::getBackendName() const
{
    return "Vulkan";
}

bool VulkanGraphicsDevice::initialize(IWindow &windowRef)
{
    window = &windowRef;
    width = windowRef.getWidth();
    height = windowRef.getHeight();

    if (!createInstance())
        return false;
    if (!createSurface(windowRef))
        return false;
    if (!pickPhysicalDevice())
        return false;
    if (!createLogicalDevice())
        return false;
    if (!createSwapchain())
        return false;
    if (!createImageViews())
        return false;
    if (!createRenderPass())
        return false;
    if (!createDepthResources())
        return false;
    if (!createFramebuffers())
        return false;
    if (!createCommandPool())
        return false;
    if (!createCommandBuffers())
        return false;
    if (!createSyncObjects())
        return false;

    triangleResourcesReady = createTrianglePipeline();
    triangleResourcesReady = triangleResourcesReady && createLinePipeline();
    if (!triangleResourcesReady)
    {
        std::fprintf(
            stderr,
            "Basic pipeline unavailable. Run `make shaders` to generate SPIR-V and restart the app.\n");
    }
    else
    {
        std::fprintf(stderr, "Basic Vulkan pipeline ready.\n");
    }

    initialized = true;
    return true;
}

void VulkanGraphicsDevice::resize(int newWidth, int newHeight)
{
    width = newWidth;
    height = newHeight;
    swapchainDirty = true;
}

int VulkanGraphicsDevice::getWidth() const
{
    return width;
}

int VulkanGraphicsDevice::getHeight() const
{
    return height;
}

void VulkanGraphicsDevice::beginFrame(uint32_t clearColor)
{
    if (!initialized)
        return;

    if (swapchainDirty)
    {
        if (!recreateSwapchain())
        {
            return;
        }
        swapchainDirty = false;
    }

    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    VkResult acquireResult = vkAcquireNextImageKHR(
        device,
        swapchain,
        UINT64_MAX,
        imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &currentImageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }

    vkResetCommandBuffer(commandBuffers[currentImageIndex], 0);
    currentClearColor = clearColor;
    opaqueSceneVertices.clear();
    transparentSceneVertices.clear();
    lineSceneVertices.clear();
    opaqueSceneVertexCount = 0;
    transparentSceneVertexCount = 0;
    lineSceneVertexCount = 0;
    frameBegun = true;
    commandBufferRecorded = false;
}

void VulkanGraphicsDevice::renderScene3D(const Camera3D &camera, const Scene3D &scene)
{
    if (!initialized || !frameBegun || !triangleResourcesReady)
    {
        return;
    }

    appendSceneVertices(camera, scene);
}

void VulkanGraphicsDevice::endFrame()
{
    if (!initialized || !frameBegun)
        return;

    if (!commandBufferRecorded)
    {
        if (!uploadSceneVertexBuffers())
        {
            return;
        }

        const bool drawTriangle = triangleResourcesReady;
        recordCommandBuffer(commandBuffers[currentImageIndex], currentImageIndex, currentClearColor, drawTriangle);
        commandBufferRecorded = true;
        if (drawTriangle && !triangleDrawLogged)
        {
            std::fprintf(stderr, "Basic draw command recorded.\n");
            triangleDrawLogged = true;
        }
    }

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentImageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

    const VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
    if (submitResult != VK_SUCCESS)
    {
        std::fprintf(stderr, "vkQueueSubmit failed with code %d.\n", submitResult);
    }
}

void VulkanGraphicsDevice::present()
{
    if (!initialized || !frameBegun)
        return;

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;

    const VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapchain();
    }

    frameBegun = false;
    commandBufferRecorded = false;
}

bool VulkanGraphicsDevice::createInstance()
{
    std::vector<const char *> requiredExtensions;
    if (window)
    {
        uint32_t extensionCount = 0;
        const char *const *extensions = window->getRequiredVulkanInstanceExtensions(extensionCount);
        requiredExtensions.assign(extensions, extensions + extensionCount);
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Phys Playground";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Phys Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.empty() ? nullptr : requiredExtensions.data();

    return vkCreateInstance(&createInfo, nullptr, &instance) == VK_SUCCESS;
}

bool VulkanGraphicsDevice::createSurface(IWindow &windowRef)
{
    return windowRef.createVulkanSurface(instance, surface);
}

bool VulkanGraphicsDevice::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (VkPhysicalDevice candidate : devices)
    {
        const QueueFamilySelection queueFamilies = findQueueFamilies(candidate, surface);
        const bool extensionsSupported = deviceSupportsRequiredExtensions(candidate);
        const SwapchainSupportDetails swapchainSupport = querySwapchainSupport(candidate, surface);
        const bool swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();

        if (queueFamilies.isComplete() && extensionsSupported && swapchainAdequate)
        {
            physicalDevice = candidate;
            graphicsQueueFamilyIndex = queueFamilies.graphicsFamily;
            presentQueueFamilyIndex = queueFamilies.presentFamily;
            return true;
        }
    }

    return false;
}

bool VulkanGraphicsDevice::createLogicalDevice()
{
    const float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector<uint32_t> uniqueFamilies = {graphicsQueueFamilyIndex};
    if (presentQueueFamilyIndex != graphicsQueueFamilyIndex)
    {
        uniqueFamilies.push_back(presentQueueFamilyIndex);
    }

    for (uint32_t familyIndex : uniqueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = familyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        return false;
    }

    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
    return true;
}

bool VulkanGraphicsDevice::createSwapchain()
{
    const SwapchainSupportDetails support = querySwapchainSupport(physicalDevice, surface);
    const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(support.formats);
    const VkPresentModeKHR presentMode = choosePresentMode(support.presentModes);
    const VkExtent2D extent = chooseExtent(support.capabilities);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
    {
        imageCount = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t queueFamilyIndices[] = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};
    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS)
    {
        return false;
    }

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;
    return true;
}

bool VulkanGraphicsDevice::createImageViews()
{
    swapchainImageViews.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

bool VulkanGraphicsDevice::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    depthFormat = findDepthFormat();

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    return vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) == VK_SUCCESS;
}

bool VulkanGraphicsDevice::createDepthResources()
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapchainExtent.width;
    imageInfo.extent.height = swapchainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &depthImage) != VK_SUCCESS)
    {
        return false;
    }

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(device, depthImage, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX)
    {
        return false;
    }

    if (vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory) != VK_SUCCESS)
    {
        return false;
    }

    if (vkBindImageMemory(device, depthImage, depthImageMemory, 0) != VK_SUCCESS)
    {
        return false;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    return vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) == VK_SUCCESS;
}

bool VulkanGraphicsDevice::createTrianglePipeline()
{
    const std::vector<char> vertexShaderCode = readFirstExistingBinary({
        "build/shaders/vulkan/basic/basic.vert.spv",
        "shaders/vulkan/basic/basic.vert.spv"});
    const std::vector<char> fragmentShaderCode = readFirstExistingBinary({
        "build/shaders/vulkan/basic/basic.frag.spv",
        "shaders/vulkan/basic/basic.frag.spv"});
    if (vertexShaderCode.empty() || fragmentShaderCode.empty())
    {
        std::fprintf(
            stderr,
            "Failed to load basic shaders. Expected files in build/shaders/vulkan/basic. Run `make shaders`.\n");
        return false;
    }

    const VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    const VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);
    if (!vertexShaderModule || !fragmentShaderModule)
    {
        if (vertexShaderModule)
            vkDestroyShaderModule(device, vertexShaderModule, nullptr);
        if (fragmentShaderModule)
            vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertexShaderModule;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = fragmentShaderModule;
    fragmentShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(TriangleVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(TriangleVertex, position);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(TriangleVertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    const std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &trianglePipelineLayout) != VK_SUCCESS)
    {
        std::fprintf(stderr, "Failed to create triangle pipeline layout.\n");
        vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(device, vertexShaderModule, nullptr);
        return false;
    }

    const auto createPipeline = [&](bool enableBlend, bool writeDepth, VkPipeline &pipeline) -> bool
    {
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = writeDepth ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = enableBlend ? VK_TRUE : VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = trianglePipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        return vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) == VK_SUCCESS;
    };

    const bool opaqueSuccess = createPipeline(false, true, opaqueTrianglePipeline);
    const bool transparentSuccess = createPipeline(true, false, transparentTrianglePipeline);
    if (!opaqueSuccess || !transparentSuccess)
    {
        std::fprintf(stderr, "Failed to create triangle graphics pipelines.\n");
    }

    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    return opaqueSuccess && transparentSuccess;
}

bool VulkanGraphicsDevice::createLinePipeline()
{
    const std::vector<char> vertexShaderCode = readFirstExistingBinary({
        "build/shaders/vulkan/basic/basic.vert.spv",
        "shaders/vulkan/basic/basic.vert.spv"});
    const std::vector<char> fragmentShaderCode = readFirstExistingBinary({
        "build/shaders/vulkan/basic/basic.frag.spv",
        "shaders/vulkan/basic/basic.frag.spv"});
    if (vertexShaderCode.empty() || fragmentShaderCode.empty())
    {
        return false;
    }

    const VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    const VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);
    if (!vertexShaderModule || !fragmentShaderModule)
    {
        if (vertexShaderModule)
            vkDestroyShaderModule(device, vertexShaderModule, nullptr);
        if (fragmentShaderModule)
            vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertexShaderModule;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = fragmentShaderModule;
    fragmentShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(TriangleVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(TriangleVertex, position);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(TriangleVertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    const std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &linePipelineLayout) != VK_SUCCESS)
    {
        vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(device, vertexShaderModule, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = linePipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    const bool success = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &linePipeline) == VK_SUCCESS;
    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    return success;
}

void VulkanGraphicsDevice::appendSceneVertices(const Camera3D &camera, const Scene3D &scene)
{
    struct EntitySortItem
    {
        const Entity3D *entity = nullptr;
        float distanceSquared = 0.0f;
    };

    std::vector<TriangleVertex> lineVertices;
    const Matrix4 viewMatrix = camera.getViewMatrix();
    const Matrix4 projectionMatrix = camera.getProjectionMatrix();
    std::vector<EntitySortItem> opaqueEntities;
    std::vector<EntitySortItem> transparentEntities;

    for (const Entity3D &entity : scene.getEntities())
    {
        EntitySortItem item;
        item.entity = &entity;
        item.distanceSquared = (entity.transform.position - camera.transform.position).lengthSquared();
        if (entity.material.isTransparent())
        {
            transparentEntities.push_back(item);
        }
        else
        {
            opaqueEntities.push_back(item);
        }
    }

    std::sort(
        transparentEntities.begin(),
        transparentEntities.end(),
        [](const EntitySortItem &a, const EntitySortItem &b)
        {
            return a.distanceSquared > b.distanceSquared;
        });

    const auto appendEntityTriangles =
        [&](const Entity3D &entity, std::vector<TriangleVertex> &vertices)
    {
        if (!entity.material.renderSolid)
        {
            return;
        }

        const Matrix4 modelMatrix = entity.transform.getModelMatrix();
        const AmbientLight &ambientLight = scene.getAmbientLight();
        for (const MeshTriangle3D &triangle : entity.mesh.triangles)
        {
            const uint32_t triangleColor = entity.material.solid.resolveColor(triangle.color);
            const std::array<float, 4> rgba = applyAmbientLighting(
                triangleColor,
                entity.material.solid.resolveEmissiveColor(),
                entity.material.solid.ambientFactor,
                ambientLight.color,
                ambientLight.intensity);

            bool triangleVisible = true;
            std::array<Vector3, 3> ndcVertices{};
            for (size_t i = 0; i < 3; ++i)
            {
                const Vector3 modelPosition = entity.mesh.vertices[triangle.indices[i]];
                const Vector3 worldPosition = modelMatrix.transformPoint(modelPosition);
                const Vector3 viewPosition = viewMatrix.transformPoint(worldPosition);
                if (-viewPosition.z < camera.nearPlane)
                {
                    triangleVisible = false;
                    break;
                }

                ndcVertices[i] = projectionMatrix.transformPoint(viewPosition);
            }

            if (!triangleVisible)
            {
                continue;
            }

            for (const Vector3 &ndcPosition : ndcVertices)
            {
                TriangleVertex vertex{};
                vertex.position[0] = ndcPosition.x;
                vertex.position[1] = ndcPosition.y;
                vertex.position[2] = ndcPosition.z;
                vertex.color[0] = rgba[0];
                vertex.color[1] = rgba[1];
                vertex.color[2] = rgba[2];
                vertex.color[3] = rgba[3];
                vertices.push_back(vertex);
            }
        }
    };

    for (const EntitySortItem &item : opaqueEntities)
    {
        appendEntityTriangles(*item.entity, opaqueSceneVertices);
    }
    for (const EntitySortItem &item : transparentEntities)
    {
        appendEntityTriangles(*item.entity, transparentSceneVertices);
    }

    for (const Entity3D &entity : scene.getEntities())
    {
        if (!entity.material.renderWireframe)
        {
            continue;
        }

        const Matrix4 modelMatrix = entity.transform.getModelMatrix();
        const AmbientLight &ambientLight = scene.getAmbientLight();
        const uint32_t lineColor = entity.material.wireframe.resolveColor();
        const std::array<float, 4> rgba = applyAmbientLighting(
            lineColor,
            entity.material.wireframe.resolveEmissiveColor(),
            entity.material.wireframe.ambientFactor,
            ambientLight.color,
            ambientLight.intensity);

        for (const MeshEdge3D &edge : entity.mesh.edges)
        {
            std::array<Vector3, 2> ndcPositions{};
            bool edgeVisible = true;
            const std::array<int, 2> indices = {edge.start, edge.end};
            for (size_t i = 0; i < indices.size(); ++i)
            {
                const Vector3 modelPosition = entity.mesh.vertices[indices[i]];
                const Vector3 worldPosition = modelMatrix.transformPoint(modelPosition);
                const Vector3 viewPosition = viewMatrix.transformPoint(worldPosition);
                if (-viewPosition.z < camera.nearPlane)
                {
                    edgeVisible = false;
                    continue;
                }

                ndcPositions[i] = projectionMatrix.transformPoint(viewPosition);
            }

            if (!edgeVisible)
            {
                continue;
            }

            for (const Vector3 &ndcPosition : ndcPositions)
            {
                TriangleVertex vertex{};
                vertex.position[0] = ndcPosition.x;
                vertex.position[1] = ndcPosition.y;
                vertex.position[2] = ndcPosition.z;
                vertex.color[0] = rgba[0];
                vertex.color[1] = rgba[1];
                vertex.color[2] = rgba[2];
                vertex.color[3] = rgba[3];
                lineVertices.push_back(vertex);
            }
        }
    }

    lineSceneVertices.insert(lineSceneVertices.end(), lineVertices.begin(), lineVertices.end());
}

bool VulkanGraphicsDevice::uploadSceneVertexBuffers()
{
    const auto uploadVertices = [&](const std::vector<TriangleVertex> &vertices, BufferHandle &bufferHandle, VkDeviceSize &bufferSize) -> bool
    {
        if (vertices.empty())
        {
            return true;
        }

        const VkDeviceSize requiredSize = sizeof(TriangleVertex) * static_cast<VkDeviceSize>(vertices.size());
        if (!bufferHandle.buffer || bufferSize < requiredSize)
        {
            if (bufferHandle.buffer)
            {
                vkDestroyBuffer(device, bufferHandle.buffer, nullptr);
                bufferHandle.buffer = VK_NULL_HANDLE;
            }
            if (bufferHandle.memory)
            {
                vkFreeMemory(device, bufferHandle.memory, nullptr);
                bufferHandle.memory = VK_NULL_HANDLE;
            }

            if (!createBuffer(
                    requiredSize,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    bufferHandle))
            {
                std::fprintf(stderr, "Failed to create scene vertex buffer.\n");
                bufferSize = 0;
                return false;
            }
            bufferSize = requiredSize;
        }

        void *data = nullptr;
        if (vkMapMemory(device, bufferHandle.memory, 0, requiredSize, 0, &data) != VK_SUCCESS)
        {
            std::fprintf(stderr, "Failed to map scene vertex buffer memory.\n");
            return false;
        }
        std::memcpy(data, vertices.data(), static_cast<size_t>(requiredSize));
        vkUnmapMemory(device, bufferHandle.memory);
        return true;
    };

    opaqueSceneVertexCount = static_cast<uint32_t>(opaqueSceneVertices.size());
    transparentSceneVertexCount = static_cast<uint32_t>(transparentSceneVertices.size());
    lineSceneVertexCount = static_cast<uint32_t>(lineSceneVertices.size());

    if (opaqueSceneVertexCount == 0 && transparentSceneVertexCount == 0 && lineSceneVertexCount == 0)
    {
        return true;
    }

    return uploadVertices(opaqueSceneVertices, opaqueSceneVertexBuffer, opaqueSceneVertexBufferSize) &&
           uploadVertices(transparentSceneVertices, transparentSceneVertexBuffer, transparentSceneVertexBufferSize) &&
           uploadVertices(lineSceneVertices, lineSceneVertexBuffer, lineSceneVertexBufferSize);
}

bool VulkanGraphicsDevice::createFramebuffers()
{
    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        std::array<VkImageView, 2> attachments = {swapchainImageViews[i], depthImageView};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

bool VulkanGraphicsDevice::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    return vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) == VK_SUCCESS;
}

bool VulkanGraphicsDevice::createCommandBuffers()
{
    commandBuffers.resize(swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    return vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) == VK_SUCCESS;
}

bool VulkanGraphicsDevice::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    return vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) == VK_SUCCESS &&
           vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) == VK_SUCCESS &&
           vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) == VK_SUCCESS;
}

bool VulkanGraphicsDevice::recreateSwapchain()
{
    if (!device || width <= 0 || height <= 0)
    {
        return false;
    }

    vkDeviceWaitIdle(device);
    destroySwapchain();
    return createSwapchain() && createImageViews() && createDepthResources() && createFramebuffers() && createCommandBuffers();
}

void VulkanGraphicsDevice::destroySwapchain()
{
    if (!device)
    {
        return;
    }

    if (!commandBuffers.empty())
    {
        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        commandBuffers.clear();
    }

    for (VkFramebuffer framebuffer : swapchainFramebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    swapchainFramebuffers.clear();

    for (VkImageView imageView : swapchainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }
    swapchainImageViews.clear();

    if (depthImageView)
    {
        vkDestroyImageView(device, depthImageView, nullptr);
        depthImageView = VK_NULL_HANDLE;
    }
    if (depthImage)
    {
        vkDestroyImage(device, depthImage, nullptr);
        depthImage = VK_NULL_HANDLE;
    }
    if (depthImageMemory)
    {
        vkFreeMemory(device, depthImageMemory, nullptr);
        depthImageMemory = VK_NULL_HANDLE;
    }

    if (swapchain)
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }
}

void VulkanGraphicsDevice::destroyDevice()
{
    if (!instance)
    {
        return;
    }

    if (device)
    {
        vkDeviceWaitIdle(device);

        if (imageAvailableSemaphore)
        {
            vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        }
        if (renderFinishedSemaphore)
        {
            vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        }
        if (inFlightFence)
        {
            vkDestroyFence(device, inFlightFence, nullptr);
        }

        destroySwapchain();

        if (commandPool)
        {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }
        if (opaqueSceneVertexBuffer.buffer)
        {
            vkDestroyBuffer(device, opaqueSceneVertexBuffer.buffer, nullptr);
        }
        if (opaqueSceneVertexBuffer.memory)
        {
            vkFreeMemory(device, opaqueSceneVertexBuffer.memory, nullptr);
        }
        if (transparentSceneVertexBuffer.buffer)
        {
            vkDestroyBuffer(device, transparentSceneVertexBuffer.buffer, nullptr);
        }
        if (transparentSceneVertexBuffer.memory)
        {
            vkFreeMemory(device, transparentSceneVertexBuffer.memory, nullptr);
        }
        if (lineSceneVertexBuffer.buffer)
        {
            vkDestroyBuffer(device, lineSceneVertexBuffer.buffer, nullptr);
        }
        if (lineSceneVertexBuffer.memory)
        {
            vkFreeMemory(device, lineSceneVertexBuffer.memory, nullptr);
        }
        if (opaqueTrianglePipeline)
        {
            vkDestroyPipeline(device, opaqueTrianglePipeline, nullptr);
        }
        if (transparentTrianglePipeline)
        {
            vkDestroyPipeline(device, transparentTrianglePipeline, nullptr);
        }
        if (linePipeline)
        {
            vkDestroyPipeline(device, linePipeline, nullptr);
        }
        if (trianglePipelineLayout)
        {
            vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);
        }
        if (linePipelineLayout)
        {
            vkDestroyPipelineLayout(device, linePipelineLayout, nullptr);
        }
        if (renderPass)
        {
            vkDestroyRenderPass(device, renderPass, nullptr);
        }

        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    if (surface)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

    vkDestroyInstance(instance, nullptr);
    instance = VK_NULL_HANDLE;
}

void VulkanGraphicsDevice::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t clearColor, bool drawTriangle)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        std::fprintf(stderr, "vkBeginCommandBuffer failed.\n");
        return;
    }

    const uint32_t vulkanColor = toVulkanColor(clearColor);
    VkClearValue clearValue{};
    clearValue.color.float32[0] = static_cast<float>(vulkanColor & 0xFF) / 255.0f;
    clearValue.color.float32[1] = static_cast<float>((vulkanColor >> 8) & 0xFF) / 255.0f;
    clearValue.color.float32[2] = static_cast<float>((vulkanColor >> 16) & 0xFF) / 255.0f;
    clearValue.color.float32[3] = static_cast<float>((vulkanColor >> 24) & 0xFF) / 255.0f;
    VkClearValue depthClearValue{};
    depthClearValue.depthStencil.depth = 1.0f;
    depthClearValue.depthStencil.stencil = 0;
    std::array<VkClearValue, 2> clearValues = {clearValue, depthClearValue};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    if (drawTriangle)
    {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(swapchainExtent.height);
        viewport.width = static_cast<float>(swapchainExtent.width);
        viewport.height = -static_cast<float>(swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkDeviceSize offsets[] = {0};
        if (opaqueTrianglePipeline && opaqueSceneVertexBuffer.buffer && opaqueSceneVertexCount > 0)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, opaqueTrianglePipeline);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &opaqueSceneVertexBuffer.buffer, offsets);
            vkCmdDraw(commandBuffer, opaqueSceneVertexCount, 1, 0, 0);
        }
        if (transparentTrianglePipeline && transparentSceneVertexBuffer.buffer && transparentSceneVertexCount > 0)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, transparentTrianglePipeline);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &transparentSceneVertexBuffer.buffer, offsets);
            vkCmdDraw(commandBuffer, transparentSceneVertexCount, 1, 0, 0);
        }
        if (linePipeline && lineSceneVertexBuffer.buffer && lineSceneVertexCount > 0)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &lineSceneVertexBuffer.buffer, offsets);
            vkCmdDraw(commandBuffer, lineSceneVertexCount, 1, 0, 0);
        }
    }
    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        std::fprintf(stderr, "vkEndCommandBuffer failed.\n");
    }
}

VkFormat VulkanGraphicsDevice::findDepthFormat() const
{
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat VulkanGraphicsDevice::findSupportedFormat(
    const std::vector<VkFormat> &candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
        {
            return format;
        }

        if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

std::vector<char> VulkanGraphicsDevice::readBinaryFile(const char *path) const
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        return {};
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize <= 0)
    {
        return {};
    }

    std::vector<char> buffer(static_cast<size_t>(fileSize));
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

VkShaderModule VulkanGraphicsDevice::createShaderModule(const std::vector<char> &code) const
{
    if (code.empty() || (code.size() % sizeof(uint32_t)) != 0)
    {
        return VK_NULL_HANDLE;
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

uint32_t VulkanGraphicsDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        const bool typeMatches = (typeFilter & (1u << i)) != 0;
        const bool propertyMatches = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if (typeMatches && propertyMatches)
        {
            return i;
        }
    }

    return UINT32_MAX;
}

bool VulkanGraphicsDevice::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    BufferHandle &bufferHandle) const
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &bufferHandle.buffer) != VK_SUCCESS)
    {
        return false;
    }

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device, bufferHandle.buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);
    if (allocInfo.memoryTypeIndex == UINT32_MAX)
    {
        vkDestroyBuffer(device, bufferHandle.buffer, nullptr);
        bufferHandle.buffer = VK_NULL_HANDLE;
        return false;
    }

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferHandle.memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(device, bufferHandle.buffer, nullptr);
        bufferHandle.buffer = VK_NULL_HANDLE;
        return false;
    }

    vkBindBufferMemory(device, bufferHandle.buffer, bufferHandle.memory, 0);
    return true;
}

VkSurfaceFormatKHR VulkanGraphicsDevice::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) const
{
    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats.front();
}

VkPresentModeKHR VulkanGraphicsDevice::choosePresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) const
{
    if (vsyncEnabled)
    {
        for (VkPresentModeKHR presentMode : availablePresentModes)
        {
            if (presentMode == VK_PRESENT_MODE_FIFO_KHR)
            {
                return presentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    for (VkPresentModeKHR presentMode : availablePresentModes)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentMode;
        }
    }

    for (VkPresentModeKHR presentMode : availablePresentModes)
    {
        if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            return presentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanGraphicsDevice::chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };

    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
    return actualExtent;
}
