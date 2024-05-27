#include "VulkanShaderUtil.h"

namespace Quasar::RendererBackend
{
    VkShaderModule ShaderModuleCreate(const VulkanDevice* device, const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device->logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to create shader module!");
        }

        return shaderModule;
    }
} // namespace Quasar::RendererBackend
