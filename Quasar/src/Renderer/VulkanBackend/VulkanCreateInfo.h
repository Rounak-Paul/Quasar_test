#pragma once

#include "VulkanTypes.inl"

namespace Quasar::RendererBackend
{
    VkImageCreateInfo VulkanImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    VkImageViewCreateInfo VulkanImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
    VkCommandPoolCreateInfo VulkanCommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags /*= 0*/);
} // namespace Quasar::RendererBackend
