#pragma once

#include <qspch.h>

#include "VulkanDevice.h"

namespace Quasar::RendererBackend
{
    class VulkanSwapchain {
        public:
        VulkanSwapchain() {}
        VulkanSwapchain(const VulkanDevice* device, const VkSurfaceKHR& surface);
        ~VulkanSwapchain() = default;

        VulkanSwapchain(const VulkanSwapchain&) = delete;
		VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

        void Destroy(const VulkanDevice* device);

        VkSwapchainKHR m_swapChain;
        std::vector<VkImage> m_swapChainImages;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImageView> m_swapChainImageViews;

        private:
        // TODO: move to VulkanImage
        void CreateImageViews(const VulkanDevice* device);
    };
} // namespace Quasar::RendererBackend
