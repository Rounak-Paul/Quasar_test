#include "VulkanBuffer.h"

namespace Quasar::RendererBackend {
void VulkanBufferCreate(VulkanContext* context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VulkanBuffer& outBuffer) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context->device.logicalDevice, &bufferInfo, nullptr, &outBuffer.buffer) != VK_SUCCESS) {
        QS_CORE_FATAL("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context->device.logicalDevice, outBuffer.buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(context, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context->device.logicalDevice, &allocInfo, nullptr, &outBuffer.memory) != VK_SUCCESS) {
        QS_CORE_FATAL("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(context->device.logicalDevice, outBuffer.buffer, outBuffer.memory, 0);
}
u32 FindMemoryType(VulkanContext* context, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(context->device.physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    QS_CORE_FATAL("failed to find suitable memory type!");
    return -1;
}

void VulkanBufferCopy(VulkanContext* context, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = SingleUseCommandBegin(context);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    SingleUseCommandEnd(context, commandBuffer);
}
}