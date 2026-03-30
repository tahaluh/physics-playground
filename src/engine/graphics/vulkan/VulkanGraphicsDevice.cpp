#include "engine/graphics/vulkan/VulkanGraphicsDevice.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <unordered_map>
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

Vector3 safeNormalized(const Vector3 &value, const Vector3 &fallback)
{
    if (value.lengthSquared() == 0.0f)
    {
        return fallback;
    }

    return value.normalized();
}

struct SceneBounds3D
{
    Vector3 min = Vector3::zero();
    Vector3 max = Vector3::zero();
    bool valid = false;
};

void expandBounds(SceneBounds3D &bounds, const Vector3 &point)
{
    if (!bounds.valid)
    {
        bounds.min = point;
        bounds.max = point;
        bounds.valid = true;
        return;
    }

    bounds.min.x = std::min(bounds.min.x, point.x);
    bounds.min.y = std::min(bounds.min.y, point.y);
    bounds.min.z = std::min(bounds.min.z, point.z);
    bounds.max.x = std::max(bounds.max.x, point.x);
    bounds.max.y = std::max(bounds.max.y, point.y);
    bounds.max.z = std::max(bounds.max.z, point.z);
}

SceneBounds3D computeSceneBounds(const Scene3D &scene)
{
    SceneBounds3D bounds;
    for (const Entity3D &entity : scene.getEntities())
    {
        if (!entity.material.renderSolid)
        {
            continue;
        }

        const Matrix4 modelMatrix = entity.transform.getModelMatrix();
        for (const Vector3 &vertex : entity.mesh.vertices)
        {
            expandBounds(bounds, modelMatrix.transformPoint(vertex));
        }
    }
    return bounds;
}

Matrix4 computeDirectionalShadowMatrix(const Scene3D &scene, const DirectionalLight &light)
{
    const SceneBounds3D bounds = computeSceneBounds(scene);
    if (!bounds.valid)
    {
        return Matrix4::identity();
    }

    const Vector3 center = (bounds.min + bounds.max) * 0.5f;
    const Vector3 extents = bounds.max - bounds.min;
    const float radius = std::max(extents.length() * 0.5f, 1.0f);
    const Vector3 lightDirection = safeNormalized(light.direction, Vector3(0.0f, -1.0f, 0.0f));
    const Vector3 upHint = std::abs(lightDirection.dot(Vector3::up())) > 0.98f ? Vector3::right() : Vector3::up();
    const Vector3 lightPosition = center - lightDirection * (radius * 2.0f);
    const Matrix4 lightView = Matrix4::lookAt(lightPosition, center, upHint);

    const std::array<Vector3, 8> corners = {
        Vector3(bounds.min.x, bounds.min.y, bounds.min.z),
        Vector3(bounds.max.x, bounds.min.y, bounds.min.z),
        Vector3(bounds.min.x, bounds.max.y, bounds.min.z),
        Vector3(bounds.max.x, bounds.max.y, bounds.min.z),
        Vector3(bounds.min.x, bounds.min.y, bounds.max.z),
        Vector3(bounds.max.x, bounds.min.y, bounds.max.z),
        Vector3(bounds.min.x, bounds.max.y, bounds.max.z),
        Vector3(bounds.max.x, bounds.max.y, bounds.max.z)};

    SceneBounds3D lightSpaceBounds;
    for (const Vector3 &corner : corners)
    {
        expandBounds(lightSpaceBounds, lightView.transformPoint(corner));
    }

    const float padding = std::max(radius * 0.15f, 0.5f);
    const float nearPlane = std::max(0.01f, -lightSpaceBounds.max.z - padding);
    const float farPlane = std::max(nearPlane + 0.01f, -lightSpaceBounds.min.z + padding);
    const Matrix4 lightProjection = Matrix4::orthographicVulkan(
        lightSpaceBounds.min.x - padding,
        lightSpaceBounds.max.x + padding,
        lightSpaceBounds.min.y - padding,
        lightSpaceBounds.max.y + padding,
        nearPlane,
        farPlane);
    return lightProjection * lightView;
}

Matrix4 computeSpotShadowMatrix(const SpotLight &light)
{
    const Vector3 lightDirection = safeNormalized(light.direction, Vector3(0.0f, -1.0f, 0.0f));
    const Vector3 upHint = std::abs(lightDirection.dot(Vector3::up())) > 0.98f ? Vector3::right() : Vector3::up();
    const float outerConeCos = Vector3::clamp(light.outerConeCos, -0.9999f, 0.9999f);
    const float fovRadians = Vector3::clamp(std::acos(outerConeCos) * 2.1f, 0.2f, 3.04159265f);
    const float nearPlane = 0.05f;
    const float farPlane = std::max(light.range, nearPlane + 0.05f);
    const Matrix4 lightView = Matrix4::lookAt(light.position, light.position + lightDirection, upHint);
    const Matrix4 lightProjection = Matrix4::perspective(fovRadians, 1.0f, nearPlane, farPlane);
    return lightProjection * lightView;
}

std::array<Matrix4, 6> computePointShadowMatrices(const PointLight &light)
{
    const float nearPlane = 0.05f;
    const float farPlane = std::max(light.range, nearPlane + 0.05f);
    const Matrix4 projection = Matrix4::perspective(3.14159265f * 0.5f, 1.0f, nearPlane, farPlane);

    const std::array<Vector3, 6> directions = {
        Vector3(1.0f, 0.0f, 0.0f),
        Vector3(-1.0f, 0.0f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f),
        Vector3(0.0f, -1.0f, 0.0f),
        Vector3(0.0f, 0.0f, 1.0f),
        Vector3(0.0f, 0.0f, -1.0f)};
    const std::array<Vector3, 6> ups = {
        Vector3(0.0f, 1.0f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f),
        Vector3(0.0f, 0.0f, -1.0f),
        Vector3(0.0f, 0.0f, 1.0f),
        Vector3(0.0f, 1.0f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f)};

    std::array<Matrix4, 6> matrices{};
    for (size_t i = 0; i < matrices.size(); ++i)
    {
        matrices[i] = projection * Matrix4::lookAt(light.position, light.position + directions[i], ups[i]);
    }

    return matrices;
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
    if (!createShadowRenderPass())
        return false;
    if (!createDepthResources())
        return false;
    if (!createShadowResources())
        return false;
    if (!createLightingResources())
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
    triangleResourcesReady = triangleResourcesReady && createShadowPipeline();
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
    shadowSceneVertices.clear();
    opaqueSceneVertexCount = 0;
    transparentSceneVertexCount = 0;
    lineSceneVertexCount = 0;
    shadowSceneVertexCount = 0;
    ambientUniform = {};
    ambientUniform.ambientColorIntensity[3] = 0.0f;
    frameBegun = true;
    commandBufferRecorded = false;
}

void VulkanGraphicsDevice::renderScene3D(const Camera3D &camera, const Scene3D &scene)
{
    if (!initialized || !frameBegun || !triangleResourcesReady)
    {
        return;
    }

    if (!updateLightingBuffers(camera, scene))
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

bool VulkanGraphicsDevice::createShadowRenderPass()
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependencies[2]{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;
    return vkCreateRenderPass(device, &renderPassInfo, nullptr, &shadowRenderPass) == VK_SUCCESS;
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

bool VulkanGraphicsDevice::createShadowResources()
{
    const uint32_t directionalLayerCount = std::max(1u, directionalShadowCount);
    const uint32_t spotLayerCount = std::max(1u, spotShadowCount);
    const uint32_t pointLayerCount = std::max(1u, pointShadowCount * VulkanGraphicsDevice::kPointShadowFaceCount);

    const auto createShadowImage =
        [&](uint32_t layers, VkImageCreateFlags flags, VkImage &image, VkDeviceMemory &memory) -> bool
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = flags;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = shadowExtent.width;
        imageInfo.extent.height = shadowExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = layers;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            return false;
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetImageMemoryRequirements(device, image, &memoryRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memoryRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (allocInfo.memoryTypeIndex == UINT32_MAX)
        {
            return false;
        }

        if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
        {
            return false;
        }

        return vkBindImageMemory(device, image, memory, 0) == VK_SUCCESS;
    };

    const auto createDepthView =
        [&](VkImage image, VkImageViewType viewType, uint32_t baseLayer, uint32_t layerCount, VkImageView &view) -> bool
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = baseLayer;
        viewInfo.subresourceRange.layerCount = layerCount;
        return vkCreateImageView(device, &viewInfo, nullptr, &view) == VK_SUCCESS;
    };

    if (!createShadowImage(directionalLayerCount, 0, directionalShadowDepthImage, directionalShadowDepthImageMemory) ||
        !createShadowImage(spotLayerCount, 0, spotShadowDepthImage, spotShadowDepthImageMemory) ||
        !createShadowImage(pointLayerCount, 0, pointShadowDepthImage, pointShadowDepthImageMemory))
    {
        return false;
    }

    if (!createDepthView(directionalShadowDepthImage, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 0, directionalLayerCount, directionalShadowDepthImageView) ||
        !createDepthView(spotShadowDepthImage, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 0, spotLayerCount, spotShadowDepthImageView) ||
        !createDepthView(pointShadowDepthImage, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 0, pointLayerCount, pointShadowDepthImageView))
    {
        return false;
    }

    directionalShadowLayerImageViews.assign(directionalLayerCount, VK_NULL_HANDLE);
    directionalShadowFramebuffers.assign(directionalLayerCount, VK_NULL_HANDLE);
    for (uint32_t layerIndex = 0; layerIndex < directionalLayerCount; ++layerIndex)
    {
        if (!createDepthView(directionalShadowDepthImage, VK_IMAGE_VIEW_TYPE_2D, layerIndex, 1, directionalShadowLayerImageViews[layerIndex]))
        {
            return false;
        }
    }

    spotShadowLayerImageViews.assign(spotLayerCount, VK_NULL_HANDLE);
    spotShadowFramebuffers.assign(spotLayerCount, VK_NULL_HANDLE);
    for (uint32_t layerIndex = 0; layerIndex < spotLayerCount; ++layerIndex)
    {
        if (!createDepthView(spotShadowDepthImage, VK_IMAGE_VIEW_TYPE_2D, layerIndex, 1, spotShadowLayerImageViews[layerIndex]))
        {
            return false;
        }
    }

    pointShadowFaceImageViews.assign(pointLayerCount, VK_NULL_HANDLE);
    pointShadowFramebuffers.assign(pointLayerCount, VK_NULL_HANDLE);
    for (uint32_t faceIndex = 0; faceIndex < pointLayerCount; ++faceIndex)
    {
        if (!createDepthView(pointShadowDepthImage, VK_IMAGE_VIEW_TYPE_2D, faceIndex, 1, pointShadowFaceImageViews[faceIndex]))
        {
            return false;
        }
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &shadowDepthSampler) != VK_SUCCESS)
    {
        return false;
    }

    const auto createShadowFramebuffer =
        [&](VkImageView attachment, VkFramebuffer &framebuffer) -> bool
    {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = shadowRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &attachment;
        framebufferInfo.width = shadowExtent.width;
        framebufferInfo.height = shadowExtent.height;
        framebufferInfo.layers = 1;
        return vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) == VK_SUCCESS;
    };

    for (uint32_t layerIndex = 0; layerIndex < directionalLayerCount; ++layerIndex)
    {
        if (!createShadowFramebuffer(directionalShadowLayerImageViews[layerIndex], directionalShadowFramebuffers[layerIndex]))
        {
            return false;
        }
    }

    for (uint32_t layerIndex = 0; layerIndex < spotLayerCount; ++layerIndex)
    {
        if (!createShadowFramebuffer(spotShadowLayerImageViews[layerIndex], spotShadowFramebuffers[layerIndex]))
        {
            return false;
        }
    }

    for (uint32_t faceIndex = 0; faceIndex < pointLayerCount; ++faceIndex)
    {
        if (!createShadowFramebuffer(pointShadowFaceImageViews[faceIndex], pointShadowFramebuffers[faceIndex]))
        {
            return false;
        }
    }

    return true;
}

bool VulkanGraphicsDevice::createLightingResources()
{
    std::array<VkDescriptorSetLayoutBinding, 10> bindings{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[5].binding = 5;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[6].binding = 6;
    bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[6].descriptorCount = 1;
    bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[7].binding = 7;
    bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[7].descriptorCount = 1;
    bindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[8].binding = 8;
    bindings[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[8].descriptorCount = 1;
    bindings[8].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[9].binding = 9;
    bindings[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[9].descriptorCount = 1;
    bindings[9].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &lightingDescriptorSetLayout) != VK_SUCCESS)
    {
        return false;
    }

    if (!createBuffer(
            sizeof(AmbientUniform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            ambientUniformBuffer))
    {
        return false;
    }
    ambientUniformBufferSize = sizeof(AmbientUniform);

    if (!createBuffer(
            sizeof(LightStorageHeader),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            directionalLightStorageBuffer))
    {
        return false;
    }
    directionalLightStorageBufferSize = sizeof(LightStorageHeader);

    if (!createBuffer(
            sizeof(LightStorageHeader),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            pointLightStorageBuffer))
    {
        return false;
    }
    pointLightStorageBufferSize = sizeof(LightStorageHeader);

    if (!createBuffer(
            sizeof(LightStorageHeader),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            spotLightStorageBuffer))
    {
        return false;
    }
    spotLightStorageBufferSize = sizeof(LightStorageHeader);

    if (!createBuffer(
            sizeof(float) * 4,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            directionalShadowMatrixStorageBuffer))
    {
        return false;
    }
    directionalShadowMatrixStorageBufferSize = sizeof(float) * 4;

    if (!createBuffer(
            sizeof(float) * 4,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            spotShadowMatrixStorageBuffer))
    {
        return false;
    }
    spotShadowMatrixStorageBufferSize = sizeof(float) * 4;

    if (!createBuffer(
            sizeof(float) * 4,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            pointShadowMatrixStorageBuffer))
    {
        return false;
    }
    pointShadowMatrixStorageBufferSize = sizeof(float) * 4;

    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = 6;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = 3;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &lightingDescriptorPool) != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = lightingDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &lightingDescriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &lightingDescriptorSet) != VK_SUCCESS)
    {
        return false;
    }

    return true;
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

    std::array<VkVertexInputAttributeDescription, 7> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(TriangleVertex, position);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(TriangleVertex, color);
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(TriangleVertex, emissive);
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(TriangleVertex, normal);
    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(TriangleVertex, worldPosition);
    attributeDescriptions[5].binding = 0;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[5].offset = offsetof(TriangleVertex, material);
    attributeDescriptions[6].binding = 0;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[6].offset = offsetof(TriangleVertex, lighting);

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

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 0;
    colorBlending.pAttachments = nullptr;

    const std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &lightingDescriptorSetLayout;

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

    std::array<VkVertexInputAttributeDescription, 7> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(TriangleVertex, position);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(TriangleVertex, color);
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(TriangleVertex, emissive);
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(TriangleVertex, normal);
    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(TriangleVertex, worldPosition);
    attributeDescriptions[5].binding = 0;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[5].offset = offsetof(TriangleVertex, material);
    attributeDescriptions[6].binding = 0;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[6].offset = offsetof(TriangleVertex, lighting);

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
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &lightingDescriptorSetLayout;

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

bool VulkanGraphicsDevice::createShadowPipeline()
{
    const std::vector<char> vertexShaderCode = readFirstExistingBinary({
        "build/shaders/vulkan/shadow/shadow.vert.spv",
        "shaders/vulkan/shadow/shadow.vert.spv"});
    if (vertexShaderCode.empty())
    {
        return false;
    }

    const VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    if (!vertexShaderModule)
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertexShaderModule;
    vertexShaderStageInfo.pName = "main";

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(TriangleVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription worldPositionAttribute{};
    worldPositionAttribute.binding = 0;
    worldPositionAttribute.location = 4;
    worldPositionAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    worldPositionAttribute.offset = offsetof(TriangleVertex, worldPosition);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = &worldPositionAttribute;

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
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = 1.25f;
    rasterizer.depthBiasSlopeFactor = 1.75f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 0;
    colorBlending.pAttachments = nullptr;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    const std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 16;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &shadowPipelineLayout) != VK_SUCCESS)
    {
        vkDestroyShaderModule(device, vertexShaderModule, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 1;
    pipelineInfo.pStages = &vertexShaderStageInfo;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = shadowPipelineLayout;
    pipelineInfo.renderPass = shadowRenderPass;
    pipelineInfo.subpass = 0;

    const bool success = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shadowPipeline) == VK_SUCCESS;
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    return success;
}

bool VulkanGraphicsDevice::updateLightingBuffers(const Camera3D &camera, const Scene3D &scene)
{
    const Matrix4 viewMatrix = camera.getViewMatrix();
    const Matrix4 projectionMatrix = camera.getProjectionMatrix();
    const std::array<float, 4> ambientColor = colorToFloat4(scene.getAmbientLight().color);
    ambientUniform.ambientColorIntensity[0] = ambientColor[0];
    ambientUniform.ambientColorIntensity[1] = ambientColor[1];
    ambientUniform.ambientColorIntensity[2] = ambientColor[2];
    ambientUniform.ambientColorIntensity[3] = std::max(scene.getAmbientLight().intensity, 0.0f);
    ambientUniform.cameraWorldPosition[0] = camera.transform.position.x;
    ambientUniform.cameraWorldPosition[1] = camera.transform.position.y;
    ambientUniform.cameraWorldPosition[2] = camera.transform.position.z;
    ambientUniform.cameraWorldPosition[3] = 1.0f;
    currentDirectionalShadowViewProjections.clear();
    currentSpotShadowViewProjections.clear();
    currentPointShadowViewProjections.clear();
    directionalShadowCount = 0;
    pointShadowCount = 0;
    spotShadowCount = 0;
    for (const DirectionalLight &light : scene.getDirectionalLights())
    {
        if (!light.enabled || light.intensity <= 0.0f)
        {
            continue;
        }

        currentDirectionalShadowViewProjections.push_back(computeDirectionalShadowMatrix(scene, light));
        ++directionalShadowCount;
    }
    for (const PointLight &light : scene.getPointLights())
    {
        if (!light.enabled || light.intensity <= 0.0f || light.range <= 0.0f)
        {
            continue;
        }

        const std::array<Matrix4, 6> pointMatrices = computePointShadowMatrices(light);
        for (uint32_t faceIndex = 0; faceIndex < VulkanGraphicsDevice::kPointShadowFaceCount; ++faceIndex)
        {
            currentPointShadowViewProjections.push_back(pointMatrices[faceIndex]);
        }
        ++pointShadowCount;
    }
    for (const SpotLight &light : scene.getSpotLights())
    {
        if (!light.enabled || light.intensity <= 0.0f || light.range <= 0.0f)
        {
            continue;
        }

        currentSpotShadowViewProjections.push_back(computeSpotShadowMatrix(light));
        ++spotShadowCount;
    }
    ambientUniform.shadowCounts[0] = static_cast<float>(directionalShadowCount);
    ambientUniform.shadowCounts[1] = static_cast<float>(pointShadowCount);
    ambientUniform.shadowCounts[2] = static_cast<float>(spotShadowCount);
    ambientUniform.shadowCounts[3] = 0.0f;
    ambientUniform.shadowBiases[0] = 0.0018f;
    ambientUniform.shadowBiases[1] = 0.0065f;
    ambientUniform.shadowBiases[2] = 0.0025f;
    ambientUniform.shadowBiases[3] = 0.0f;
    std::memcpy(ambientUniform.viewMatrix, viewMatrix.m.data(), sizeof(ambientUniform.viewMatrix));
    std::memcpy(ambientUniform.projectionMatrix, projectionMatrix.m.data(), sizeof(ambientUniform.projectionMatrix));

    const uint32_t desiredDirectionalLayers = std::max(1u, directionalShadowCount);
    const uint32_t desiredSpotLayers = std::max(1u, spotShadowCount);
    const uint32_t desiredPointLayers = std::max(1u, pointShadowCount * VulkanGraphicsDevice::kPointShadowFaceCount);
    if (!directionalShadowDepthImage ||
        directionalShadowLayerImageViews.size() != desiredDirectionalLayers ||
        spotShadowLayerImageViews.size() != desiredSpotLayers ||
        pointShadowFaceImageViews.size() != desiredPointLayers)
    {
        for (VkFramebuffer framebuffer : directionalShadowFramebuffers)
        {
            if (framebuffer)
            {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            }
        }
        directionalShadowFramebuffers.clear();
        for (VkImageView imageView : directionalShadowLayerImageViews)
        {
            if (imageView)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }
        }
        directionalShadowLayerImageViews.clear();
        if (directionalShadowDepthImageView)
        {
            vkDestroyImageView(device, directionalShadowDepthImageView, nullptr);
            directionalShadowDepthImageView = VK_NULL_HANDLE;
        }
        if (directionalShadowDepthImage)
        {
            vkDestroyImage(device, directionalShadowDepthImage, nullptr);
            directionalShadowDepthImage = VK_NULL_HANDLE;
        }
        if (directionalShadowDepthImageMemory)
        {
            vkFreeMemory(device, directionalShadowDepthImageMemory, nullptr);
            directionalShadowDepthImageMemory = VK_NULL_HANDLE;
        }

        for (VkFramebuffer framebuffer : spotShadowFramebuffers)
        {
            if (framebuffer)
            {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            }
        }
        spotShadowFramebuffers.clear();
        for (VkImageView imageView : spotShadowLayerImageViews)
        {
            if (imageView)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }
        }
        spotShadowLayerImageViews.clear();
        if (spotShadowDepthImageView)
        {
            vkDestroyImageView(device, spotShadowDepthImageView, nullptr);
            spotShadowDepthImageView = VK_NULL_HANDLE;
        }
        if (spotShadowDepthImage)
        {
            vkDestroyImage(device, spotShadowDepthImage, nullptr);
            spotShadowDepthImage = VK_NULL_HANDLE;
        }
        if (spotShadowDepthImageMemory)
        {
            vkFreeMemory(device, spotShadowDepthImageMemory, nullptr);
            spotShadowDepthImageMemory = VK_NULL_HANDLE;
        }

        for (VkFramebuffer framebuffer : pointShadowFramebuffers)
        {
            if (framebuffer)
            {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            }
        }
        pointShadowFramebuffers.clear();
        for (VkImageView imageView : pointShadowFaceImageViews)
        {
            if (imageView)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }
        }
        pointShadowFaceImageViews.clear();
        if (pointShadowDepthImageView)
        {
            vkDestroyImageView(device, pointShadowDepthImageView, nullptr);
            pointShadowDepthImageView = VK_NULL_HANDLE;
        }
        if (pointShadowDepthImage)
        {
            vkDestroyImage(device, pointShadowDepthImage, nullptr);
            pointShadowDepthImage = VK_NULL_HANDLE;
        }
        if (pointShadowDepthImageMemory)
        {
            vkFreeMemory(device, pointShadowDepthImageMemory, nullptr);
            pointShadowDepthImageMemory = VK_NULL_HANDLE;
        }

        if (!createShadowResources())
        {
            return false;
        }
    }

    const auto updateBufferData =
        [&](BufferHandle &bufferHandle,
            VkDeviceSize &bufferSize,
            VkBufferUsageFlags usage,
            const void *data,
            VkDeviceSize requiredSize) -> bool
    {
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
                    usage,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    bufferHandle))
            {
                return false;
            }
            bufferSize = requiredSize;
        }

        void *mappedData = nullptr;
        if (vkMapMemory(device, bufferHandle.memory, 0, requiredSize, 0, &mappedData) != VK_SUCCESS)
        {
            return false;
        }
        std::memcpy(mappedData, data, static_cast<size_t>(requiredSize));
        vkUnmapMemory(device, bufferHandle.memory);
        return true;
    };

    std::vector<GpuDirectionalLight> directionalLights;
    directionalLights.reserve(scene.getDirectionalLights().size());
    for (const DirectionalLight &light : scene.getDirectionalLights())
    {
        if (!light.enabled || light.intensity <= 0.0f)
        {
            continue;
        }

        const Vector3 lightDirection = safeNormalized(light.direction, Vector3(0.0f, -1.0f, 0.0f));
        const std::array<float, 4> lightColor = colorToFloat4(light.color);
        GpuDirectionalLight gpuLight{};
        gpuLight.direction[0] = lightDirection.x;
        gpuLight.direction[1] = lightDirection.y;
        gpuLight.direction[2] = lightDirection.z;
        gpuLight.direction[3] = 0.0f;
        gpuLight.colorIntensity[0] = lightColor[0];
        gpuLight.colorIntensity[1] = lightColor[1];
        gpuLight.colorIntensity[2] = lightColor[2];
        gpuLight.colorIntensity[3] = std::max(light.intensity, 0.0f);
        directionalLights.push_back(gpuLight);
    }

    std::vector<char> directionalBytes(sizeof(LightStorageHeader) + directionalLights.size() * sizeof(GpuDirectionalLight));
    auto *directionalHeader = reinterpret_cast<LightStorageHeader *>(directionalBytes.data());
    directionalHeader->count = static_cast<uint32_t>(directionalLights.size());
    if (!directionalLights.empty())
    {
        std::memcpy(
            directionalBytes.data() + sizeof(LightStorageHeader),
            directionalLights.data(),
            directionalLights.size() * sizeof(GpuDirectionalLight));
    }

    std::vector<GpuPointLight> pointLights;
    pointLights.reserve(scene.getPointLights().size());
    for (const PointLight &light : scene.getPointLights())
    {
        if (!light.enabled || light.intensity <= 0.0f || light.range <= 0.0f)
        {
            continue;
        }

        const std::array<float, 4> lightColor = colorToFloat4(light.color);
        GpuPointLight gpuLight{};
        gpuLight.positionRange[0] = light.position.x;
        gpuLight.positionRange[1] = light.position.y;
        gpuLight.positionRange[2] = light.position.z;
        gpuLight.positionRange[3] = light.range;
        gpuLight.colorIntensity[0] = lightColor[0];
        gpuLight.colorIntensity[1] = lightColor[1];
        gpuLight.colorIntensity[2] = lightColor[2];
        gpuLight.colorIntensity[3] = light.intensity;
        pointLights.push_back(gpuLight);
    }

    std::vector<char> pointBytes(sizeof(LightStorageHeader) + pointLights.size() * sizeof(GpuPointLight));
    auto *pointHeader = reinterpret_cast<LightStorageHeader *>(pointBytes.data());
    pointHeader->count = static_cast<uint32_t>(pointLights.size());
    if (!pointLights.empty())
    {
        std::memcpy(
            pointBytes.data() + sizeof(LightStorageHeader),
            pointLights.data(),
            pointLights.size() * sizeof(GpuPointLight));
    }

    std::vector<GpuSpotLight> spotLights;
    spotLights.reserve(scene.getSpotLights().size());
    for (const SpotLight &light : scene.getSpotLights())
    {
        if (!light.enabled || light.intensity <= 0.0f || light.range <= 0.0f)
        {
            continue;
        }

        const Vector3 lightDirection = safeNormalized(light.direction, Vector3(0.0f, -1.0f, 0.0f));
        const std::array<float, 4> lightColor = colorToFloat4(light.color);
        GpuSpotLight gpuLight{};
        gpuLight.positionRange[0] = light.position.x;
        gpuLight.positionRange[1] = light.position.y;
        gpuLight.positionRange[2] = light.position.z;
        gpuLight.positionRange[3] = light.range;
        gpuLight.directionInnerCone[0] = lightDirection.x;
        gpuLight.directionInnerCone[1] = lightDirection.y;
        gpuLight.directionInnerCone[2] = lightDirection.z;
        gpuLight.directionInnerCone[3] = light.innerConeCos;
        gpuLight.colorIntensity[0] = lightColor[0];
        gpuLight.colorIntensity[1] = lightColor[1];
        gpuLight.colorIntensity[2] = lightColor[2];
        gpuLight.colorIntensity[3] = light.intensity;
        gpuLight.outerConeCos[0] = light.outerConeCos;
        gpuLight.outerConeCos[1] = 0.0f;
        gpuLight.outerConeCos[2] = 0.0f;
        gpuLight.outerConeCos[3] = 0.0f;
        spotLights.push_back(gpuLight);
    }

    std::vector<char> spotBytes(sizeof(LightStorageHeader) + spotLights.size() * sizeof(GpuSpotLight));
    auto *spotHeader = reinterpret_cast<LightStorageHeader *>(spotBytes.data());
    spotHeader->count = static_cast<uint32_t>(spotLights.size());
    if (!spotLights.empty())
    {
        std::memcpy(
            spotBytes.data() + sizeof(LightStorageHeader),
            spotLights.data(),
            spotLights.size() * sizeof(GpuSpotLight));
    }

    std::vector<float> directionalShadowMatrixFloats(currentDirectionalShadowViewProjections.size() * 16, 0.0f);
    for (size_t matrixIndex = 0; matrixIndex < currentDirectionalShadowViewProjections.size(); ++matrixIndex)
    {
        std::memcpy(
            directionalShadowMatrixFloats.data() + matrixIndex * 16,
            currentDirectionalShadowViewProjections[matrixIndex].m.data(),
            sizeof(float) * 16);
    }

    std::vector<float> spotShadowMatrixFloats(currentSpotShadowViewProjections.size() * 16, 0.0f);
    for (size_t matrixIndex = 0; matrixIndex < currentSpotShadowViewProjections.size(); ++matrixIndex)
    {
        std::memcpy(
            spotShadowMatrixFloats.data() + matrixIndex * 16,
            currentSpotShadowViewProjections[matrixIndex].m.data(),
            sizeof(float) * 16);
    }

    std::vector<float> pointShadowMatrixFloats(currentPointShadowViewProjections.size() * 16, 0.0f);
    for (size_t matrixIndex = 0; matrixIndex < currentPointShadowViewProjections.size(); ++matrixIndex)
    {
        std::memcpy(
            pointShadowMatrixFloats.data() + matrixIndex * 16,
            currentPointShadowViewProjections[matrixIndex].m.data(),
            sizeof(float) * 16);
    }

    if (!updateBufferData(
            ambientUniformBuffer,
            ambientUniformBufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            &ambientUniform,
            sizeof(AmbientUniform)))
    {
        return false;
    }

    if (!updateBufferData(
            directionalLightStorageBuffer,
            directionalLightStorageBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            directionalBytes.data(),
            static_cast<VkDeviceSize>(directionalBytes.size())))
    {
        return false;
    }

    if (!updateBufferData(
            pointLightStorageBuffer,
            pointLightStorageBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            pointBytes.data(),
            static_cast<VkDeviceSize>(pointBytes.size())))
    {
        return false;
    }

    if (!updateBufferData(
            spotLightStorageBuffer,
            spotLightStorageBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            spotBytes.data(),
            static_cast<VkDeviceSize>(spotBytes.size())))
    {
        return false;
    }

    const VkDeviceSize directionalShadowMatrixBytes = std::max<VkDeviceSize>(sizeof(float) * 4, directionalShadowMatrixFloats.size() * sizeof(float));
    if (!updateBufferData(
            directionalShadowMatrixStorageBuffer,
            directionalShadowMatrixStorageBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            directionalShadowMatrixFloats.empty() ? &ambientUniform.shadowCounts[3] : directionalShadowMatrixFloats.data(),
            directionalShadowMatrixBytes))
    {
        return false;
    }

    const VkDeviceSize spotShadowMatrixBytes = std::max<VkDeviceSize>(sizeof(float) * 4, spotShadowMatrixFloats.size() * sizeof(float));
    if (!updateBufferData(
            spotShadowMatrixStorageBuffer,
            spotShadowMatrixStorageBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            spotShadowMatrixFloats.empty() ? &ambientUniform.shadowCounts[3] : spotShadowMatrixFloats.data(),
            spotShadowMatrixBytes))
    {
        return false;
    }

    const VkDeviceSize pointShadowMatrixBytes = std::max<VkDeviceSize>(sizeof(float) * 4, pointShadowMatrixFloats.size() * sizeof(float));
    if (!updateBufferData(
            pointShadowMatrixStorageBuffer,
            pointShadowMatrixStorageBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            pointShadowMatrixFloats.empty() ? &ambientUniform.shadowCounts[3] : pointShadowMatrixFloats.data(),
            pointShadowMatrixBytes))
    {
        return false;
    }

    std::array<VkDescriptorBufferInfo, 7> bufferInfos{};
    bufferInfos[0].buffer = ambientUniformBuffer.buffer;
    bufferInfos[0].offset = 0;
    bufferInfos[0].range = sizeof(AmbientUniform);
    bufferInfos[1].buffer = directionalLightStorageBuffer.buffer;
    bufferInfos[1].offset = 0;
    bufferInfos[1].range = static_cast<VkDeviceSize>(directionalBytes.size());
    bufferInfos[2].buffer = pointLightStorageBuffer.buffer;
    bufferInfos[2].offset = 0;
    bufferInfos[2].range = static_cast<VkDeviceSize>(pointBytes.size());
    bufferInfos[3].buffer = spotLightStorageBuffer.buffer;
    bufferInfos[3].offset = 0;
    bufferInfos[3].range = static_cast<VkDeviceSize>(spotBytes.size());
    bufferInfos[4].buffer = directionalShadowMatrixStorageBuffer.buffer;
    bufferInfos[4].offset = 0;
    bufferInfos[4].range = directionalShadowMatrixBytes;
    bufferInfos[5].buffer = spotShadowMatrixStorageBuffer.buffer;
    bufferInfos[5].offset = 0;
    bufferInfos[5].range = spotShadowMatrixBytes;
    bufferInfos[6].buffer = pointShadowMatrixStorageBuffer.buffer;
    bufferInfos[6].offset = 0;
    bufferInfos[6].range = pointShadowMatrixBytes;
    VkDescriptorImageInfo directionalShadowImageInfo{};
    directionalShadowImageInfo.sampler = shadowDepthSampler;
    directionalShadowImageInfo.imageView = directionalShadowDepthImageView;
    directionalShadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkDescriptorImageInfo spotShadowImageInfo{};
    spotShadowImageInfo.sampler = shadowDepthSampler;
    spotShadowImageInfo.imageView = spotShadowDepthImageView;
    spotShadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkDescriptorImageInfo pointShadowImageInfo{};
    pointShadowImageInfo.sampler = shadowDepthSampler;
    pointShadowImageInfo.imageView = pointShadowDepthImageView;
    pointShadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::array<VkWriteDescriptorSet, 10> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = lightingDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfos[0];

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = lightingDescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &bufferInfos[1];

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = lightingDescriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &bufferInfos[2];

    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = lightingDescriptorSet;
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pBufferInfo = &bufferInfos[3];
    descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[4].dstSet = lightingDescriptorSet;
    descriptorWrites[4].dstBinding = 4;
    descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[4].descriptorCount = 1;
    descriptorWrites[4].pBufferInfo = &bufferInfos[4];
    descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[5].dstSet = lightingDescriptorSet;
    descriptorWrites[5].dstBinding = 5;
    descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[5].descriptorCount = 1;
    descriptorWrites[5].pBufferInfo = &bufferInfos[5];
    descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[6].dstSet = lightingDescriptorSet;
    descriptorWrites[6].dstBinding = 6;
    descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[6].descriptorCount = 1;
    descriptorWrites[6].pBufferInfo = &bufferInfos[6];
    descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[7].dstSet = lightingDescriptorSet;
    descriptorWrites[7].dstBinding = 7;
    descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[7].descriptorCount = 1;
    descriptorWrites[7].pImageInfo = &directionalShadowImageInfo;
    descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[8].dstSet = lightingDescriptorSet;
    descriptorWrites[8].dstBinding = 8;
    descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[8].descriptorCount = 1;
    descriptorWrites[8].pImageInfo = &spotShadowImageInfo;
    descriptorWrites[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[9].dstSet = lightingDescriptorSet;
    descriptorWrites[9].dstBinding = 9;
    descriptorWrites[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[9].descriptorCount = 1;
    descriptorWrites[9].pImageInfo = &pointShadowImageInfo;
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    return true;
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
        std::vector<const MeshTriangle3D *> sortedTriangles;
        if (entity.material.solid.isTransparent())
        {
            struct TriangleSortItem
            {
                const MeshTriangle3D *triangle = nullptr;
                float distanceSquared = 0.0f;
            };

            std::vector<TriangleSortItem> triangleItems;
            triangleItems.reserve(entity.mesh.triangles.size());
            for (const MeshTriangle3D &triangle : entity.mesh.triangles)
            {
                const Vector3 a = modelMatrix.transformPoint(entity.mesh.vertices[triangle.indices[0]]);
                const Vector3 b = modelMatrix.transformPoint(entity.mesh.vertices[triangle.indices[1]]);
                const Vector3 c = modelMatrix.transformPoint(entity.mesh.vertices[triangle.indices[2]]);
                const Vector3 center = (a + b + c) / 3.0f;
                triangleItems.push_back({&triangle, (center - camera.transform.position).lengthSquared()});
            }

            std::sort(
                triangleItems.begin(),
                triangleItems.end(),
                [](const TriangleSortItem &a, const TriangleSortItem &b)
                {
                    return a.distanceSquared > b.distanceSquared;
                });

            sortedTriangles.reserve(triangleItems.size());
            for (const TriangleSortItem &item : triangleItems)
            {
                sortedTriangles.push_back(item.triangle);
            }
        }

        const auto emitTriangle =
            [&](const MeshTriangle3D &triangle)
        {
            const uint32_t triangleColor = entity.material.solid.resolveBaseColor(triangle.color);
            const std::array<float, 4> baseColor = colorToFloat4(triangleColor);
            const std::array<float, 4> emissiveColor = colorToFloat4(entity.material.solid.resolveEmissiveColor());
            bool triangleVisible = true;
            std::array<Vector3, 3> worldVertices{};
            std::array<Vector3, 3> ndcVertices{};
            for (size_t i = 0; i < 3; ++i)
            {
                const Vector3 modelPosition = entity.mesh.vertices[triangle.indices[i]];
                const Vector3 worldPosition = modelMatrix.transformPoint(modelPosition);
                worldVertices[i] = worldPosition;
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
                return;
            }

            Vector3 surfaceNormal = (worldVertices[1] - worldVertices[0]).cross(worldVertices[2] - worldVertices[0]).normalized();
            if (surfaceNormal.lengthSquared() == 0.0f)
            {
                surfaceNormal = Vector3::up();
            }

            for (size_t vertexIndex = 0; vertexIndex < ndcVertices.size(); ++vertexIndex)
            {
                const Vector3 &positionValue = ndcVertices[vertexIndex];
                TriangleVertex vertex{};
                vertex.position[0] = positionValue.x;
                vertex.position[1] = positionValue.y;
                vertex.position[2] = positionValue.z;
                vertex.color[0] = baseColor[0];
                vertex.color[1] = baseColor[1];
                vertex.color[2] = baseColor[2];
                vertex.color[3] = baseColor[3];
                vertex.emissive[0] = emissiveColor[0];
                vertex.emissive[1] = emissiveColor[1];
                vertex.emissive[2] = emissiveColor[2];
                vertex.emissive[3] = 0.0f;
                Vector3 finalNormal = surfaceNormal;
                const int meshVertexIndex = triangle.indices[vertexIndex];
                if (meshVertexIndex >= 0 && static_cast<size_t>(meshVertexIndex) < entity.mesh.vertexNormals.size())
                {
                    finalNormal = modelMatrix.transformVector(entity.mesh.vertexNormals[meshVertexIndex]).normalized();
                    if (finalNormal.lengthSquared() == 0.0f)
                    {
                        finalNormal = surfaceNormal;
                    }
                }

                vertex.normal[0] = finalNormal.x;
                vertex.normal[1] = finalNormal.y;
                vertex.normal[2] = finalNormal.z;
                vertex.normal[3] = 0.0f;
                vertex.worldPosition[0] = worldVertices[vertexIndex].x;
                vertex.worldPosition[1] = worldVertices[vertexIndex].y;
                vertex.worldPosition[2] = worldVertices[vertexIndex].z;
                vertex.worldPosition[3] = 1.0f;
                vertex.material[0] = Vector3::clamp(entity.material.solid.ambientFactor, 0.0f, 1.0f);
                vertex.material[1] = Vector3::clamp(entity.material.solid.diffuseFactor, 0.0f, 1.0f);
                vertex.material[2] = entity.material.solid.unlit ? 1.0f : 0.0f;
                vertex.material[3] = entity.material.solid.doubleSidedLighting ? 1.0f : 0.0f;
                vertex.lighting[0] = Vector3::clamp(entity.material.solid.metallic, 0.0f, 1.0f);
                vertex.lighting[1] = Vector3::clamp(entity.material.solid.roughness, 0.0f, 1.0f);
                vertex.lighting[2] = 0.0f;
                vertex.lighting[3] = 0.0f;
                vertices.push_back(vertex);
            }
        };

        if (!sortedTriangles.empty())
        {
            for (const MeshTriangle3D *triangle : sortedTriangles)
            {
                emitTriangle(*triangle);
            }
            return;
        }

        for (const MeshTriangle3D &triangle : entity.mesh.triangles)
        {
            emitTriangle(triangle);
        }
    };

    const auto appendShadowCasterTriangles =
        [&](const Entity3D &entity)
    {
        if (!entity.material.renderSolid)
        {
            return;
        }

        const Matrix4 modelMatrix = entity.transform.getModelMatrix();
        for (const MeshTriangle3D &triangle : entity.mesh.triangles)
        {
            for (size_t vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
            {
                const Vector3 worldPosition = modelMatrix.transformPoint(entity.mesh.vertices[triangle.indices[vertexIndex]]);
                TriangleVertex vertex{};
                vertex.worldPosition[0] = worldPosition.x;
                vertex.worldPosition[1] = worldPosition.y;
                vertex.worldPosition[2] = worldPosition.z;
                vertex.worldPosition[3] = 1.0f;
                shadowSceneVertices.push_back(vertex);
            }
        }
    };

    for (const EntitySortItem &item : opaqueEntities)
    {
        appendEntityTriangles(*item.entity, opaqueSceneVertices);
        appendShadowCasterTriangles(*item.entity);
    }
    for (const EntitySortItem &item : transparentEntities)
    {
        appendEntityTriangles(*item.entity, transparentSceneVertices);
        appendShadowCasterTriangles(*item.entity);
    }

    for (const Entity3D &entity : scene.getEntities())
    {
        if (!entity.material.renderWireframe)
        {
            continue;
        }

        const Matrix4 modelMatrix = entity.transform.getModelMatrix();
        const uint32_t lineColor = entity.material.wireframe.resolveBaseColor();
        const std::array<float, 4> rgba = colorToFloat4(lineColor);
        const std::array<float, 4> emissiveColor = colorToFloat4(entity.material.wireframe.resolveEmissiveColor());

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
                vertex.emissive[0] = emissiveColor[0];
                vertex.emissive[1] = emissiveColor[1];
                vertex.emissive[2] = emissiveColor[2];
                vertex.emissive[3] = 0.0f;
                vertex.normal[0] = 0.0f;
                vertex.normal[1] = 0.0f;
                vertex.normal[2] = 0.0f;
                vertex.normal[3] = 0.0f;
                vertex.worldPosition[0] = 0.0f;
                vertex.worldPosition[1] = 0.0f;
                vertex.worldPosition[2] = 0.0f;
                vertex.worldPosition[3] = 1.0f;
                vertex.material[0] = Vector3::clamp(entity.material.wireframe.ambientFactor, 0.0f, 1.0f);
                vertex.material[1] = 0.0f;
                vertex.material[2] = entity.material.wireframe.unlit ? 1.0f : 0.0f;
                vertex.material[3] = entity.material.wireframe.doubleSidedLighting ? 1.0f : 0.0f;
                vertex.lighting[0] = Vector3::clamp(entity.material.wireframe.metallic, 0.0f, 1.0f);
                vertex.lighting[1] = Vector3::clamp(entity.material.wireframe.roughness, 0.0f, 1.0f);
                vertex.lighting[2] = 0.0f;
                vertex.lighting[3] = 0.0f;
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
    shadowSceneVertexCount = static_cast<uint32_t>(shadowSceneVertices.size());

    if (opaqueSceneVertexCount == 0 && transparentSceneVertexCount == 0 && lineSceneVertexCount == 0 && shadowSceneVertexCount == 0)
    {
        return true;
    }

    return uploadVertices(opaqueSceneVertices, opaqueSceneVertexBuffer, opaqueSceneVertexBufferSize) &&
           uploadVertices(transparentSceneVertices, transparentSceneVertexBuffer, transparentSceneVertexBufferSize) &&
           uploadVertices(lineSceneVertices, lineSceneVertexBuffer, lineSceneVertexBufferSize) &&
           uploadVertices(shadowSceneVertices, shadowSceneVertexBuffer, shadowSceneVertexBufferSize);
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
        if (shadowSceneVertexBuffer.buffer)
        {
            vkDestroyBuffer(device, shadowSceneVertexBuffer.buffer, nullptr);
        }
        if (shadowSceneVertexBuffer.memory)
        {
            vkFreeMemory(device, shadowSceneVertexBuffer.memory, nullptr);
        }
        if (ambientUniformBuffer.buffer)
        {
            vkDestroyBuffer(device, ambientUniformBuffer.buffer, nullptr);
        }
        if (ambientUniformBuffer.memory)
        {
            vkFreeMemory(device, ambientUniformBuffer.memory, nullptr);
        }
        if (directionalLightStorageBuffer.buffer)
        {
            vkDestroyBuffer(device, directionalLightStorageBuffer.buffer, nullptr);
        }
        if (directionalLightStorageBuffer.memory)
        {
            vkFreeMemory(device, directionalLightStorageBuffer.memory, nullptr);
        }
        if (pointLightStorageBuffer.buffer)
        {
            vkDestroyBuffer(device, pointLightStorageBuffer.buffer, nullptr);
        }
        if (pointLightStorageBuffer.memory)
        {
            vkFreeMemory(device, pointLightStorageBuffer.memory, nullptr);
        }
        if (spotLightStorageBuffer.buffer)
        {
            vkDestroyBuffer(device, spotLightStorageBuffer.buffer, nullptr);
        }
        if (spotLightStorageBuffer.memory)
        {
            vkFreeMemory(device, spotLightStorageBuffer.memory, nullptr);
        }
        if (directionalShadowMatrixStorageBuffer.buffer)
        {
            vkDestroyBuffer(device, directionalShadowMatrixStorageBuffer.buffer, nullptr);
        }
        if (directionalShadowMatrixStorageBuffer.memory)
        {
            vkFreeMemory(device, directionalShadowMatrixStorageBuffer.memory, nullptr);
        }
        if (spotShadowMatrixStorageBuffer.buffer)
        {
            vkDestroyBuffer(device, spotShadowMatrixStorageBuffer.buffer, nullptr);
        }
        if (spotShadowMatrixStorageBuffer.memory)
        {
            vkFreeMemory(device, spotShadowMatrixStorageBuffer.memory, nullptr);
        }
        if (pointShadowMatrixStorageBuffer.buffer)
        {
            vkDestroyBuffer(device, pointShadowMatrixStorageBuffer.buffer, nullptr);
        }
        if (pointShadowMatrixStorageBuffer.memory)
        {
            vkFreeMemory(device, pointShadowMatrixStorageBuffer.memory, nullptr);
        }
        for (VkFramebuffer framebuffer : directionalShadowFramebuffers)
        {
            if (framebuffer)
            {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            }
        }
        for (VkFramebuffer framebuffer : spotShadowFramebuffers)
        {
            if (framebuffer)
            {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            }
        }
        for (VkFramebuffer framebuffer : pointShadowFramebuffers)
        {
            if (framebuffer)
            {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            }
        }
        if (shadowDepthSampler)
        {
            vkDestroySampler(device, shadowDepthSampler, nullptr);
        }
        if (directionalShadowDepthImageView)
        {
            vkDestroyImageView(device, directionalShadowDepthImageView, nullptr);
        }
        for (VkImageView imageView : directionalShadowLayerImageViews)
        {
            if (imageView)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }
        }
        if (spotShadowDepthImageView)
        {
            vkDestroyImageView(device, spotShadowDepthImageView, nullptr);
        }
        for (VkImageView imageView : spotShadowLayerImageViews)
        {
            if (imageView)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }
        }
        if (pointShadowDepthImageView)
        {
            vkDestroyImageView(device, pointShadowDepthImageView, nullptr);
        }
        for (VkImageView imageView : pointShadowFaceImageViews)
        {
            if (imageView)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }
        }
        if (directionalShadowDepthImage)
        {
            vkDestroyImage(device, directionalShadowDepthImage, nullptr);
        }
        if (spotShadowDepthImage)
        {
            vkDestroyImage(device, spotShadowDepthImage, nullptr);
        }
        if (pointShadowDepthImage)
        {
            vkDestroyImage(device, pointShadowDepthImage, nullptr);
        }
        if (directionalShadowDepthImageMemory)
        {
            vkFreeMemory(device, directionalShadowDepthImageMemory, nullptr);
        }
        if (spotShadowDepthImageMemory)
        {
            vkFreeMemory(device, spotShadowDepthImageMemory, nullptr);
        }
        if (pointShadowDepthImageMemory)
        {
            vkFreeMemory(device, pointShadowDepthImageMemory, nullptr);
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
        if (shadowPipeline)
        {
            vkDestroyPipeline(device, shadowPipeline, nullptr);
        }
        if (trianglePipelineLayout)
        {
            vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);
        }
        if (linePipelineLayout)
        {
            vkDestroyPipelineLayout(device, linePipelineLayout, nullptr);
        }
        if (shadowPipelineLayout)
        {
            vkDestroyPipelineLayout(device, shadowPipelineLayout, nullptr);
        }
        if (lightingDescriptorPool)
        {
            vkDestroyDescriptorPool(device, lightingDescriptorPool, nullptr);
        }
        if (lightingDescriptorSetLayout)
        {
            vkDestroyDescriptorSetLayout(device, lightingDescriptorSetLayout, nullptr);
        }
        if (renderPass)
        {
            vkDestroyRenderPass(device, renderPass, nullptr);
        }
        if (shadowRenderPass)
        {
            vkDestroyRenderPass(device, shadowRenderPass, nullptr);
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

    if (drawTriangle && shadowPipeline && shadowSceneVertexBuffer.buffer && shadowSceneVertexCount > 0)
    {
        const auto renderShadowPass = [&](VkFramebuffer framebuffer, const Matrix4 &shadowMatrix)
        {
            if (!framebuffer)
            {
                return;
            }

            VkClearValue shadowClearValue{};
            shadowClearValue.depthStencil.depth = 1.0f;
            shadowClearValue.depthStencil.stencil = 0;

            VkRenderPassBeginInfo shadowRenderPassInfo{};
            shadowRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            shadowRenderPassInfo.renderPass = shadowRenderPass;
            shadowRenderPassInfo.framebuffer = framebuffer;
            shadowRenderPassInfo.renderArea.offset = {0, 0};
            shadowRenderPassInfo.renderArea.extent = shadowExtent;
            shadowRenderPassInfo.clearValueCount = 1;
            shadowRenderPassInfo.pClearValues = &shadowClearValue;

            vkCmdBeginRenderPass(commandBuffer, &shadowRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport shadowViewport{};
            shadowViewport.x = 0.0f;
            shadowViewport.y = 0.0f;
            shadowViewport.width = static_cast<float>(shadowExtent.width);
            shadowViewport.height = static_cast<float>(shadowExtent.height);
            shadowViewport.minDepth = 0.0f;
            shadowViewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &shadowViewport);

            VkRect2D shadowScissor{};
            shadowScissor.offset = {0, 0};
            shadowScissor.extent = shadowExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &shadowScissor);

            VkDeviceSize shadowOffsets[] = {0};
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
            vkCmdPushConstants(
                commandBuffer,
                shadowPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(float) * 16,
                shadowMatrix.m.data());
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shadowSceneVertexBuffer.buffer, shadowOffsets);
            vkCmdDraw(commandBuffer, shadowSceneVertexCount, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffer);
        };

        for (uint32_t shadowIndex = 0; shadowIndex < directionalShadowCount; ++shadowIndex)
        {
            renderShadowPass(directionalShadowFramebuffers[shadowIndex], currentDirectionalShadowViewProjections[shadowIndex]);
        }
        for (uint32_t shadowIndex = 0; shadowIndex < spotShadowCount; ++shadowIndex)
        {
            renderShadowPass(spotShadowFramebuffers[shadowIndex], currentSpotShadowViewProjections[shadowIndex]);
        }
        for (uint32_t pointIndex = 0; pointIndex < pointShadowCount; ++pointIndex)
        {
            for (uint32_t faceIndex = 0; faceIndex < VulkanGraphicsDevice::kPointShadowFaceCount; ++faceIndex)
            {
                const uint32_t layerIndex = pointIndex * VulkanGraphicsDevice::kPointShadowFaceCount + faceIndex;
                renderShadowPass(pointShadowFramebuffers[layerIndex], currentPointShadowViewProjections[layerIndex]);
            }
        }
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
            if (lightingDescriptorSet)
            {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipelineLayout, 0, 1, &lightingDescriptorSet, 0, nullptr);
            }
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &opaqueSceneVertexBuffer.buffer, offsets);
            vkCmdDraw(commandBuffer, opaqueSceneVertexCount, 1, 0, 0);
        }
        if (transparentTrianglePipeline && transparentSceneVertexBuffer.buffer && transparentSceneVertexCount > 0)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, transparentTrianglePipeline);
            if (lightingDescriptorSet)
            {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipelineLayout, 0, 1, &lightingDescriptorSet, 0, nullptr);
            }
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &transparentSceneVertexBuffer.buffer, offsets);
            vkCmdDraw(commandBuffer, transparentSceneVertexCount, 1, 0, 0);
        }
        if (linePipeline && lineSceneVertexBuffer.buffer && lineSceneVertexCount > 0)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
            if (lightingDescriptorSet)
            {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipelineLayout, 0, 1, &lightingDescriptorSet, 0, nullptr);
            }
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
