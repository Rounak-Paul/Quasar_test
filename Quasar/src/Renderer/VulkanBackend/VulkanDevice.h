#pragma once

#include <qspch.h>

namespace Quasar::RendererBackend {
    typedef struct VulkanSwapchainSupportInfo {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    } VulkanSwapchainSupportInfo;

    class VulkanDevice {
        public:
        VulkanDevice();
        ~VulkanDevice() = default;

        VulkanDevice(const VulkanDevice&) = delete;
		VulkanDevice& operator=(const VulkanDevice&) = delete;

        b8 Create(const VkInstance& vkInstance, const VkSurfaceKHR& vkSurface, VkAllocationCallbacks* vkAllocator);
        void Destroy();
        b8 DetectDepthFormat();
        static void QuerySwapchainSupport(
            VkPhysicalDevice activePhysicalDevice,
            VkSurfaceKHR surface,
            VulkanSwapchainSupportInfo* outSupportInfo
        );

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

        private:
        VkAllocationCallbacks* allocator;

        b8 SelectPhysicalDevice(const VkInstance& vkInstance, const VkSurfaceKHR& vkSurface, b8 discreteGPU);
    };
}