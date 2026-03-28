#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "engine/graphics/IGraphicsDevice.h"

class VulkanGraphicsDevice : public IGraphicsDevice
{
public:
    struct TriangleVertex
    {
        float position[3];
        float color[4];
    };

    struct BufferHandle
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    ~VulkanGraphicsDevice() override;

    GraphicsBackend getBackend() const override;
    const char *getBackendName() const override;

    void configurePresentation(bool enabledVsync, int requestedTargetFrameRate) override
    {
        vsyncEnabled = enabledVsync;
        targetFrameRate = requestedTargetFrameRate;
    }
    bool initialize(IWindow &window) override;
    void resize(int width, int height) override;
    int getWidth() const override;
    int getHeight() const override;

    void beginFrame(uint32_t clearColor) override;
    void renderScene3D(const Camera3D &camera, const Scene3D &scene) override;
    void endFrame() override;
    void present() override;

private:
    bool createInstance();
    bool createSurface(IWindow &window);
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapchain();
    bool createImageViews();
    bool createRenderPass();
    bool createDepthResources();
    bool createTrianglePipeline();
    bool updateSceneVertexBuffer(const Camera3D &camera, const Scene3D &scene);
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();
    bool recreateSwapchain();
    void destroySwapchain();
    void destroyDevice();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t clearColor, bool drawTriangle);
    std::vector<char> readBinaryFile(const char *path) const;
    VkShaderModule createShaderModule(const std::vector<char> &code) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkFormat findDepthFormat() const;
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    bool createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        BufferHandle &bufferHandle) const;
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) const;
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) const;
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;

    IWindow *window = nullptr;
    int width = 0;
    int height = 0;
    bool initialized = false;
    bool swapchainDirty = false;
    bool frameBegun = false;
    bool commandBufferRecorded = false;
    bool triangleResourcesReady = false;
    bool triangleDrawLogged = false;
    bool vsyncEnabled = true;
    int targetFrameRate = 60;
    uint32_t currentImageIndex = 0;
    uint32_t currentClearColor = 0xFF000000;

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent = {};
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout trianglePipelineLayout = VK_NULL_HANDLE;
    VkPipeline trianglePipeline = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    BufferHandle sceneVertexBuffer;
    VkDeviceSize sceneVertexBufferSize = 0;
    uint32_t sceneVertexCount = 0;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
};
