#pragma once

#include <qspch.h>
#include "VulkanTypes.inl"

namespace Quasar::RendererBackend
{
    VkImageView VulkanImageViewCreate(VulkanContext* context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels);
} // namespace Quasar::RendererBackend