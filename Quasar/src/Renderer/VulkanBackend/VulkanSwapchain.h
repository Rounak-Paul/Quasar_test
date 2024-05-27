#pragma once

#include <qspch.h>

#include "VulkanTypes.inl"

namespace Quasar::RendererBackend
{
    b8 VulkanSwapchainCreate(VulkanContext* context, VulkanSwapchain* outSwapchain);
    void VulkanSwapchainDestroy(VulkanContext* context, VulkanSwapchain* swapchain);
    b8 VulkanSwapchainRecreate(VulkanContext* context, VulkanSwapchain* outSwapchain);

} // namespace Quasar::RendererBackend
