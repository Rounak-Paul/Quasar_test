#pragma once

#include <qspch.h>
#include "VulkanTypes.inl"
#include "VulkanCommandBuffer.h"

namespace Quasar::RendererBackend {
    void VulkanBufferCreate(VulkanContext* context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VulkanBuffer& outBuffer);
    u32 FindMemoryType(VulkanContext* context, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void VulkanBufferCopy(VulkanContext* context, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size); 
}