#include "VulkanSwapchain.h"

#include <Core/Application.h>

namespace Quasar::RendererBackend
{
    VkSurfaceFormatKHR __SwapSurfaceFormatChoose(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR __SwapPresentModeChoose(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D __SwapExtentChoose(const VkSurfaceCapabilitiesKHR& capabilities);
    void __ImageViewsCreate(VulkanContext* context, VulkanSwapchain* outSwapchain);
    b8 __Create(VulkanContext* context, VulkanSwapchain* outSwapchain);
    void __Destroy(VulkanContext* context, VulkanSwapchain* swapchain);

    b8 VulkanSwapchainCreate(VulkanContext* context, VulkanSwapchain* outSwapchain) {
        if (__Create(context, outSwapchain)) {
            return true;
        }
        return false;
    }

    void VulkanSwapchainDestroy(VulkanContext* context, VulkanSwapchain* swapchain) {
        __Destroy(context, swapchain);
    }

    b8 VulkanSwapchainRecreate(VulkanContext* context, VulkanSwapchain* outSwapchain) {
        __Destroy(context, outSwapchain);
        if (__Create(context, outSwapchain)) {
            return true;
        }
        return false;
    }

    b8 __Create(VulkanContext* context, VulkanSwapchain* outSwapchain) {
        VkSurfaceFormatKHR surfaceFormat = __SwapSurfaceFormatChoose(context->device.swapchainSupport.formats);
        VkPresentModeKHR presentMode = __SwapPresentModeChoose(context->device.swapchainSupport.presentModes);
        VkExtent2D extent = __SwapExtentChoose(context->device.swapchainSupport.capabilities);

        uint32_t imageCount = context->device.swapchainSupport.capabilities.minImageCount + 1;
        if (context->device.swapchainSupport.capabilities.maxImageCount > 0 && imageCount > context->device.swapchainSupport.capabilities.maxImageCount) {
            imageCount = context->device.swapchainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = context->surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        u32 queueFamilyIndices[] = {context->device.graphicsQueueIndex, context->device.presentQueueIndex};

        if (context->device.graphicsQueueIndex != context->device.presentQueueIndex) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = context->device.swapchainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(context->device.logicalDevice, &createInfo, nullptr, &outSwapchain->handle) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to create swap chain!");
            return false;
        }

        vkGetSwapchainImagesKHR(context->device.logicalDevice, outSwapchain->handle, &imageCount, nullptr);
        outSwapchain->swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(context->device.logicalDevice, outSwapchain->handle, &imageCount, outSwapchain->swapChainImages.data());

        outSwapchain->swapChainImageFormat = surfaceFormat.format;
        outSwapchain->swapChainExtent = extent;

        // TODO: move to VulkanImage
        __ImageViewsCreate(context, outSwapchain);
        return true;
    }

    void __Destroy(VulkanContext* context, VulkanSwapchain* swapchain) {
        // TODO: move to VulkanImage
        for (auto it : swapchain->swapChainImageViews) {
            vkDestroyImageView(context->device.logicalDevice, it, nullptr);
        }

        swapchain->swapChainImages.clear();
        vkDestroySwapchainKHR(context->device.logicalDevice, swapchain->handle, nullptr);
    }

    // TODO: move to VulkanImage
    void __ImageViewsCreate(VulkanContext* context, VulkanSwapchain* outSwapchain) {
        outSwapchain->swapChainImageViews.resize(outSwapchain->swapChainImages.size());

        for (size_t i = 0; i < outSwapchain->swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = outSwapchain->swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = outSwapchain->swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(context->device.logicalDevice, &createInfo, nullptr, &outSwapchain->swapChainImageViews[i]) != VK_SUCCESS) {
                QS_CORE_FATAL("failed to create image views!");
            }
        }
    }

    VkSurfaceFormatKHR __SwapSurfaceFormatChoose(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR __SwapPresentModeChoose(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        // VK_PRESENT_MODE_IMMEDIATE_KHR = 0,
        // VK_PRESENT_MODE_MAILBOX_KHR = 1,
        // VK_PRESENT_MODE_FIFO_KHR = 2,
        // VK_PRESENT_MODE_FIFO_RELAXED_KHR = 3,
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D __SwapExtentChoose(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(QS_MAIN_WINDOW.GetGLFWwindow(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = QS_CLAMP(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = QS_CLAMP(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
} // namespace Quasar::RendererBackend