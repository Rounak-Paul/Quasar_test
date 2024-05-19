#pragma once

#include <qspch.h>

#include "VulkanDevice.h"

namespace Quasar::RendererBackend
{
    class VulkanSwapchain {
        public:
        VulkanSwapchain(const VulkanDevice* device, const VkSurfaceKHR& surface);
        ~VulkanSwapchain() = default;

        void Destroy(const VulkanDevice* device);

        VkSwapchainKHR m_swapChain;
        std::vector<VkImage> m_swapChainImages;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
    };
} // namespace Quasar::RendererBackend
