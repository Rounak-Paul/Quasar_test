#pragma once

#include <qspch.h>
#include "VulkanTypes.inl"

namespace Quasar::RendererBackend
{
    void VulkanImageCreate(
        VulkanContext* context, 
        u32 width, 
        u32 height, 
        u32 mipLevels, 
        VkSampleCountFlagBits numSamples, 
        VkFormat format, 
        VkImageTiling tiling, 
        VkImageUsageFlags usage, 
        VkMemoryPropertyFlags properties, 
        VulkanImage& outImage);

    VkImageView VulkanImageViewCreate(
        VulkanContext* context, 
        VkImage image, 
        VkFormat format, 
        VkImageAspectFlags aspectFlags, 
        u32 mipLevels);
    
    void VulkanImageLayoutTransition(
        VulkanContext* context, 
        VulkanImage image, 
        VkFormat format, 
        VkImageLayout oldLayout, 
        VkImageLayout newLayout,
        u32 mipLevels);

    void CopyBufferToImage(VulkanContext* context, VkBuffer buffer, VulkanImage image);

} // namespace Quasar::RendererBackend