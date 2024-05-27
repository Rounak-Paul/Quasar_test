#pragma once

#include <qspch.h>
#include "VulkanTypes.inl"

namespace Quasar::RendererBackend {
    VkCommandBuffer SingleUseCommandBegin(VulkanContext* context);
    void SingleUseCommandEnd(VulkanContext* context, VkCommandBuffer commandBuffer);
}