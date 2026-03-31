#pragma once

#include <array>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "engine/graphics/IGraphicsDevice.h"
#include "engine/math/Matrix4.h"
#include "engine/physics/BroadPhase.h"

class VulkanGraphicsDevice : public IGraphicsDevice, public BroadPhaseCompute
{
public:
    static constexpr uint32_t kPointShadowFaceCount = 6;

    struct TriangleVertex
    {
        float position[3];
        float color[4];
        float barycentric[3];
        float emissive[4];
        float normal[4];
        float worldPosition[4];
        float material[4];
        float lighting[4];
    };

    struct InstancedMeshVertex
    {
        float position[3];
        float color[4];
        float barycentric[3];
        float normal[4];
    };

    struct InstancedMeshInstance
    {
        float modelRow0[4];
        float modelRow1[4];
        float modelRow2[4];
        float modelRow3[4];
        float color[4];
        float emissive[4];
        float material[4];
        float lighting[4];
        float position[4];
        float rotation[4];
        float scale[4];
        float linearVelocity[4];
        float angularVelocity[4];
    };

    struct GpuDirectionalLight
    {
        float direction[4];
        float colorIntensity[4];
    };

    struct GpuPointLight
    {
        float positionRange[4];
        float colorIntensity[4];
    };

    struct GpuSpotLight
    {
        float positionRange[4];
        float directionInnerCone[4];
        float colorIntensity[4];
        float outerConeCos[4];
    };

    struct AmbientUniform
    {
        float ambientColorIntensity[4];
        float cameraWorldPosition[4];
        float shadowBiases[4];
        float shadowCounts[4];
        float viewMatrix[16];
        float projectionMatrix[16];
        float debugFlags[4];
    };

    struct LightStorageHeader
    {
        uint32_t count = 0;
        uint32_t padding0 = 0;
        uint32_t padding1 = 0;
        uint32_t padding2 = 0;
    };

    struct BufferHandle
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    struct InstancedBatch
    {
        std::string key;
        int chunkX = 0;
        int chunkY = 0;
        int chunkZ = 0;
        std::vector<InstancedMeshVertex> meshVertices;
        std::vector<InstancedMeshInstance> instances;
        Vector3 boundsCenter = Vector3::zero();
        float boundsRadius = 0.0f;
        BufferHandle meshVertexBuffer;
        BufferHandle instanceBuffer;
        VkDeviceSize meshVertexBufferSize = 0;
        VkDeviceSize instanceBufferSize = 0;
        uint32_t meshVertexCount = 0;
        uint32_t instanceCount = 0;
        bool simulateOnGpu = false;
        VkDescriptorSet simulationDescriptorSet = VK_NULL_HANDLE;
    };

    struct CachedSceneBounds
    {
        const Scene *scene = nullptr;
        uint64_t revision = 0;
        Vector3 min = Vector3::zero();
        Vector3 max = Vector3::zero();
        bool valid = false;
    };

    ~VulkanGraphicsDevice() override;

    GraphicsBackend getBackend() const override;
    const char *getBackendName() const override;
    const char *getDeviceName() const override;

    void configurePresentation(bool enabledVsync, int requestedTargetFrameRate) override
    {
        vsyncEnabled = enabledVsync;
        targetFrameRate = requestedTargetFrameRate;
    }
    void setLightDebugOverlayEnabled(bool enabled) override
    {
        lightDebugOverlayEnabled = enabled;
    }
    void setWireframeOverlayEnabled(bool enabled) override
    {
        wireframeOverlayEnabled = enabled;
    }
    BroadPhaseCompute *getBroadPhaseCompute() override
    {
        return broadPhasePipeline ? this : nullptr;
    }
    bool initialize(IWindow &window) override;
    void resize(int width, int height) override;
    int getWidth() const override;
    int getHeight() const override;

    void beginFrame(uint32_t clearColor) override;
    void renderScene(const Camera &camera, const Scene &scene) override;
    void endFrame() override;
    void present() override;
    bool computeCandidatePairs(
        const std::vector<BroadPhaseBodyInput> &inputs,
        std::vector<BroadPhaseCandidatePair> &pairs) override;

private:
    bool createInstance();
    bool createSurface(IWindow &window);
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapchain();
    bool createImageViews();
    bool createRenderPass();
    bool createShadowRenderPass();
    bool createDepthResources();
    bool createShadowResources();
    bool createLightingResources();
    bool createTrianglePipeline();
    bool createInstancedTrianglePipeline();
    bool createLinePipeline();
    bool createShadowPipeline();
    bool createInstancedShadowPipeline();
    bool createSimulationResources();
    bool createSimulationPipeline();
    bool createBroadPhaseResources();
    bool createBroadPhasePipeline();
    void appendSceneVertices(const Camera &camera, const Scene &scene);
    bool updateLightDebugVertexBuffer(const Camera &camera, const Scene &scene);
    bool updateSceneTransformBuffers(const Camera &camera, const Scene &scene);
    bool updateSceneMaterialBuffers(const Scene &scene);
    bool updateLightingBuffers(const Camera &camera, const Scene &scene);
    CachedSceneBounds getCachedSceneBounds(const Scene &scene);
    bool uploadSceneVertexBuffers();
    bool uploadSceneTransformBuffers();
    void destroyInstancedBatches();
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
    bool sceneBuffersDirty = true;
    bool sceneTransformBuffersDirty = false;
    bool simulationDispatchDirty = false;
    bool shadowMapsDirty = true;
    bool triangleResourcesReady = false;
    bool triangleDrawLogged = false;
    bool lightDebugOverlayEnabled = false;
    bool cachedLightDebugOverlayEnabled = false;
    bool lightDebugBufferDirty = true;
    bool vsyncEnabled = true;
    bool wireframeOverlayEnabled = false;
    int targetFrameRate = 60;
    uint32_t currentImageIndex = 0;
    uint32_t currentClearColor = 0xFF000000;

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    std::string deviceName;
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
    VkImage directionalShadowDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory directionalShadowDepthImageMemory = VK_NULL_HANDLE;
    VkImageView directionalShadowDepthImageView = VK_NULL_HANDLE;
    VkSampler shadowDepthSampler = VK_NULL_HANDLE;
    std::vector<VkImageView> directionalShadowLayerImageViews;
    std::vector<VkFramebuffer> directionalShadowFramebuffers;
    VkImage spotShadowDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory spotShadowDepthImageMemory = VK_NULL_HANDLE;
    VkImageView spotShadowDepthImageView = VK_NULL_HANDLE;
    std::vector<VkImageView> spotShadowLayerImageViews;
    std::vector<VkFramebuffer> spotShadowFramebuffers;
    VkImage pointShadowDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory pointShadowDepthImageMemory = VK_NULL_HANDLE;
    VkImageView pointShadowDepthImageView = VK_NULL_HANDLE;
    std::vector<VkImageView> pointShadowFaceImageViews;
    std::vector<VkFramebuffer> pointShadowFramebuffers;
    VkExtent2D shadowExtent = {2048, 2048};

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkRenderPass shadowRenderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout lightingDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout simulationDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout broadPhaseDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool lightingDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorPool simulationDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorPool broadPhaseDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet lightingDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet broadPhaseDescriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout trianglePipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout linePipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout simulationPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout broadPhasePipelineLayout = VK_NULL_HANDLE;
    VkPipeline opaqueTrianglePipeline = VK_NULL_HANDLE;
    VkPipeline opaqueInstancedTrianglePipeline = VK_NULL_HANDLE;
    VkPipeline transparentTrianglePipeline = VK_NULL_HANDLE;
    VkPipeline linePipeline = VK_NULL_HANDLE;
    VkPipeline shadowPipeline = VK_NULL_HANDLE;
    VkPipeline shadowInstancedPipeline = VK_NULL_HANDLE;
    VkPipeline simulationPipeline = VK_NULL_HANDLE;
    VkPipeline broadPhasePipeline = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    BufferHandle opaqueSceneVertexBuffer;
    BufferHandle transparentSceneVertexBuffer;
    BufferHandle lineSceneVertexBuffer;
    BufferHandle lightDebugLineVertexBuffer;
    BufferHandle shadowSceneVertexBuffer;
    BufferHandle ambientUniformBuffer;
    BufferHandle broadPhaseInputBuffer;
    BufferHandle broadPhaseOutputBuffer;
    BufferHandle broadPhaseCounterBuffer;
    BufferHandle directionalLightStorageBuffer;
    BufferHandle pointLightStorageBuffer;
    BufferHandle spotLightStorageBuffer;
    BufferHandle directionalShadowMatrixStorageBuffer;
    BufferHandle spotShadowMatrixStorageBuffer;
    BufferHandle pointShadowMatrixStorageBuffer;
    VkDeviceSize ambientUniformBufferSize = 0;
    VkDeviceSize broadPhaseInputBufferSize = 0;
    VkDeviceSize broadPhaseOutputBufferSize = 0;
    VkDeviceSize broadPhaseCounterBufferSize = 0;
    VkDeviceSize directionalLightStorageBufferSize = 0;
    VkDeviceSize pointLightStorageBufferSize = 0;
    VkDeviceSize spotLightStorageBufferSize = 0;
    VkDeviceSize directionalShadowMatrixStorageBufferSize = 0;
    VkDeviceSize spotShadowMatrixStorageBufferSize = 0;
    VkDeviceSize pointShadowMatrixStorageBufferSize = 0;
    VkDeviceSize opaqueSceneVertexBufferSize = 0;
    VkDeviceSize transparentSceneVertexBufferSize = 0;
    VkDeviceSize lineSceneVertexBufferSize = 0;
    VkDeviceSize lightDebugLineVertexBufferSize = 0;
    VkDeviceSize shadowSceneVertexBufferSize = 0;
    uint32_t opaqueSceneVertexCount = 0;
    uint32_t transparentSceneVertexCount = 0;
    uint32_t lineSceneVertexCount = 0;
    uint32_t lightDebugLineVertexCount = 0;
    uint32_t shadowSceneVertexCount = 0;
    std::vector<TriangleVertex> opaqueSceneVertices;
    std::vector<TriangleVertex> transparentSceneVertices;
    std::vector<TriangleVertex> lineSceneVertices;
    std::vector<TriangleVertex> lightDebugLineVertices;
    std::vector<TriangleVertex> shadowSceneVertices;
    std::vector<InstancedBatch> opaqueInstancedBatches;
    AmbientUniform ambientUniform = {};
    const Scene *cachedScene = nullptr;
    const Scene *cachedLightDebugScene = nullptr;
    uint64_t cachedSceneRevision = 0;
    uint64_t cachedSceneTransformRevision = 0;
    uint64_t cachedSceneSimulationRevision = 0;
    uint64_t cachedLightDebugSceneRevision = 0;
    float currentSimulationDeltaTime = 0.0f;
    CachedSceneBounds cachedShadowSceneBounds;
    Matrix4 currentCullViewMatrix = Matrix4::identity();
    float currentCullFovRadians = 60.0f * 3.14159265f / 180.0f;
    float currentCullAspectRatio = 4.0f / 3.0f;
    float currentCullNearPlane = 0.1f;
    float currentCullFarPlane = 100.0f;
    std::vector<Matrix4> currentDirectionalShadowViewProjections;
    std::vector<Matrix4> currentSpotShadowViewProjections;
    std::vector<Matrix4> currentPointShadowViewProjections;
    uint32_t directionalShadowCount = 0;
    uint32_t spotShadowCount = 0;
    uint32_t pointShadowCount = 0;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
};
