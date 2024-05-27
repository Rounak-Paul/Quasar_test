#pragma once

#include <qspch.h>

#include "VulkanDevice.h"

namespace Quasar::RendererBackend
{
    VkShaderModule ShaderModuleCreate(const VulkanDevice* device, const std::vector<char>& code);
} // namespace Quasar::RendererBackend
