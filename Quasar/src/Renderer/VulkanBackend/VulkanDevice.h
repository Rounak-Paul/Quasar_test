#pragma once

#include <qspch.h>

namespace Quasar::RendererBackend {
    typedef struct VulkanSwapchainSupportInfo {
        VkSurfaceCapabilitiesKHR capabilities;
        u32 formatCount;
        std::vector<VkSurfaceFormatKHR> formats;
        u32 presentModeCount;
        std::vector<VkPresentModeKHR> presentModes;
    } VulkanSwapchainSupportInfo;

    class VulkanDevice {
        public:
        VulkanDevice(const VkInstance& vkInstance, const VkSurfaceKHR& vkSurface, VkAllocationCallbacks* allocator);
        ~VulkanDevice() = default;

        void Destroy();
        b8 DetectDepthFormat();

        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_logicalDevice = VK_NULL_HANDLE;
        VulkanSwapchainSupportInfo m_swapchainSupport;
        i32 m_graphicsQueueIndex;
        i32 m_presentQueueIndex;
        i32 m_transferQueueIndex;
        b8 m_supportsDeviceLocalHostVisible;

        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;
        VkQueue m_transferQueue;

        VkCommandPool m_graphicsCommandPool;

        VkPhysicalDeviceProperties m_properties;
        VkPhysicalDeviceFeatures m_features;
        VkPhysicalDeviceMemoryProperties m_memory;

        VkFormat m_depthFormat;

        private:
        VkAllocationCallbacks* m_allocator;

        b8 SelectPhysicalDevice(const VkInstance& vkInstance, const VkSurfaceKHR& vkSurface, b8 discreteGPU);
    };
}