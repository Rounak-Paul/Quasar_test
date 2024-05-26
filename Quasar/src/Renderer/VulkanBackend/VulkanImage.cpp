#include "VulkanImage.h"

namespace Quasar::RendererBackend
{
    VkImageView VulkanImageViewCreate(VulkanContext* context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(context->device.logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to create image view!");
        }

        return imageView;
    }
} // namespace Quasar::RendererBackend
