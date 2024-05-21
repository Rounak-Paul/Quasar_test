#pragma once
#include <qspch.h>

namespace Quasar::RendererBackend {

typedef struct VulkanSwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
} VulkanSwapchainSupportInfo;

typedef struct VulkanDevice {
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;

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
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
} VulkanSwapchain;

typedef struct VulkanContext {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkAllocationCallbacks* allocator;
    VulkanDevice* device;
    
    VulkanSwapchain* swapchain;
    VkRenderPass renderPass;

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