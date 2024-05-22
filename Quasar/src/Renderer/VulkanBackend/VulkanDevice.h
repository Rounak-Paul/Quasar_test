#pragma once

#include <qspch.h>
#include "VulkanTypes.inl"

namespace Quasar::RendererBackend {
    b8 VulkanDeviceCreate(VulkanContext* context, VulkanDevice* outDevice);
    void VulkanDeviceDestroy(VulkanContext* context, VulkanDevice* device);
    b8 VulkanDeviceDetectDepthFormat(VulkanDevice* device);
    void VulkanDeviceQuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VulkanSwapchainSupportInfo* outSupportInfo);
}