#pragma once

#include <qspch.h>
#include "VulkanTypes.inl"

namespace Quasar::RendererBackend {
    void VulkanBufferCreate(VulkanContext* context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    u32 FindMemoryType(VulkanContext* context, uint32_t typeFilter, VkMemoryPropertyFlags properties);
}