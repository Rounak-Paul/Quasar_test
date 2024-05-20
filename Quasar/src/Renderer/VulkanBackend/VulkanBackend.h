#pragma once

#include <qspch.h>

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanShaderUtil.h"

namespace Quasar::RendererBackend
{
    class Backend {
        public:
        Backend();
        ~Backend() = default;

        Backend(const Backend&) = delete;
		Backend& operator=(const Backend&) = delete;

        b8 Init(String appName, u16 w, u16 h);
        void Shutdown();

        private:
        VkInstance m_vkInstance;
        VkAllocationCallbacks* m_allocator = nullptr;
        VkSurfaceKHR m_vkSurface;
        VulkanDevice* m_device = nullptr;
        VulkanSwapchain* m_swapchain = nullptr;
        VkRenderPass m_renderPass;
        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_graphicsPipeline;
        std::vector<VkFramebuffer> m_swapChainFramebuffers;

        u16 m_width, m_height;

        private:
        std::vector<const char*> GetRequiredExtensions();
        b8 CheckValidationLayerSupport();
        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        std::vector<const char*> m_validationLayers = { 
            "VK_LAYER_KHRONOS_validation" 
            // ,"VK_LAYER_LUNARG_api_dump" // For all vulkan calls
        };
        void GraphicsPipelineCreate();
        void RenderPassCreate();
        void FramebuffersCreate();
    };
} // namespace Quasar
