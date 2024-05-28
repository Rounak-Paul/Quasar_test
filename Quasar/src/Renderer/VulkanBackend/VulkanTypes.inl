#pragma once
#include <qspch.h>

#include <vk_mem_alloc.h>

namespace Quasar::RendererBackend {

#define VK_CHECK(expr)                  \
{                                      \
    assert(expr == VK_SUCCESS);        \
} 

#define MAX_FRAMES_IN_FLIGHT 2

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

typedef struct VulkanImage {
    VkImage image;
    VmaAllocation allocation;
    VkImageView view;
    VkExtent3D extent;
    VkFormat format;
} VulkanImage;

typedef struct VulkanBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
} VulkanBuffer;

typedef struct VulkanTexture {
    VulkanImage texture;
    VkSampler sampler;
    u32 mipLevels;
} VulkanTexture;

struct FrameData {
	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;

    DescriptorAllocatorGrowable _frameDescriptors;
    DeletionQueue _deletionQueue;

    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
};

typedef struct VulkanSwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
} VulkanSwapchainSupportInfo;

typedef struct VulkanDevice {
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    VulkanSwapchainSupportInfo swapchainSupport;
    u32 graphicsQueueIndex;
    u32 presentQueueIndex;
    u32 transferQueueIndex;
    b8 supportsDeviceLocalHostVisible;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;

    VkCommandPool graphicsCommandPool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat depthFormat;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
} VulkanDevice;

typedef struct VulkanSwapchain {
    VkSwapchainKHR handle;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
    VulkanImage depthAttachment;
    VulkanImage colorAttachment;
} VulkanSwapchain;

typedef struct VulkanPipeline {
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline handle;
} VulkanPipeline;

typedef struct VulkanContext {
    b8 resizeRequested;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkAllocationCallbacks* allocator;
    VmaAllocator vmaAllocator;
    VulkanDevice device;
    VulkanPipeline graphicsPipeline;

    VulkanSwapchain swapchain;

    // draw resources
    VulkanImage drawImage;
    VulkanImage depthImage;

    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    
    
    VkRenderPass renderpass;

    

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    VulkanBuffer stagingBuffer;

    std::vector<VulkanBuffer> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    u16 width, height, frameIndex;
    f32 dt;
} VulkanContext;

}