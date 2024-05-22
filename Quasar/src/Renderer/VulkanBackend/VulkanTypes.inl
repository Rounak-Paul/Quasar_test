#pragma once
#include <qspch.h>

namespace Quasar::RendererBackend {

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
} VulkanDevice;

typedef struct VulkanSwapchain {
    VkSwapchainKHR handle;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
} VulkanSwapchain;

typedef struct VulkanContext {
    b8 recreatingSwapchain;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkAllocationCallbacks* allocator;
    VulkanDevice device;
    
    VulkanSwapchain swapchain;
    VkRenderPass renderpass;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    u16 width, height;
} VulkanContext;

}