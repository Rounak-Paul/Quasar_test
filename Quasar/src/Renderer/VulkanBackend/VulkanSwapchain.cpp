#include "VulkanSwapchain.h"

#include <Core/Application.h>

namespace Quasar::RendererBackend
{
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VulkanSwapchain::VulkanSwapchain(const VulkanDevice* device, const VkSurfaceKHR& surface) {
        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(device->m_swapchainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(device->m_swapchainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(device->m_swapchainSupport.capabilities);

        uint32_t imageCount = device->m_swapchainSupport.capabilities.minImageCount + 1;
        if (device->m_swapchainSupport.capabilities.maxImageCount > 0 && imageCount > device->m_swapchainSupport.capabilities.maxImageCount) {
            imageCount = device->m_swapchainSupport.capabilities.maxImageCount;
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

        u32 queueFamilyIndices[] = {device->m_graphicsQueueIndex, device->m_presentQueueIndex};

        if (device->m_graphicsQueueIndex != device->m_presentQueueIndex) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = device->m_swapchainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device->m_logicalDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device->m_logicalDevice, m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device->m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;

        // TODO: move to VulkanImage
        CreateImageViews(device);
    }

    void VulkanSwapchain::Destroy(const VulkanDevice* device) {
        // TODO: move to VulkanImage
        for (auto imageView : m_swapChainImageViews) {
            vkDestroyImageView(device->m_logicalDevice, imageView, nullptr);
        }

        m_swapChainImages.clear();
        vkDestroySwapchainKHR(device->m_logicalDevice, m_swapChain, nullptr);
    }

    // TODO: move to VulkanImage
    void VulkanSwapchain::CreateImageViews(const VulkanDevice* device) {
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device->m_logicalDevice, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
                QS_CORE_FATAL("failed to create image views!");
            }
        }
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(QS_MAIN_WINDOW.GetGLFWwindow(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
} // namespace Quasar::RendererBackend