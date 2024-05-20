#pragma once

#include <qspch.h>

#include "VulkanDevice.h"

namespace Quasar::RendererBackend
{
    VkShaderModule CreateShaderModule(const VulkanDevice* device, const std::vector<char>& code);
} // namespace Quasar::RendererBackend
