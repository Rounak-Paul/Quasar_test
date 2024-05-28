#pragma once
#include <qspch.h>

namespace Quasar::RendererBackend
{
    void VulkanGraphicsPipelineCreate(VulkanContext* context);
    void VulkanPipelineDestroy(VulkanContext* context, VulkanPipeline& pipeline);
}