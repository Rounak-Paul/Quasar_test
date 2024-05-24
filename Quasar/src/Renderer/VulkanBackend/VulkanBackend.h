#pragma once

#include <qspch.h>
#include "VulkanTypes.inl"

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanShaderUtil.h"
#include "VulkanBuffer.h"

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

        void DrawFrame();
        void Resize();

        b8 framebufferResized = false;

        const std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
        };

        const std::vector<u16> indices = {
            0, 1, 2, 2, 3, 0
        };

        private:
        VulkanContext* context = nullptr;

        private:
        std::vector<const char*> GetRequiredExtensions();
        b8 CheckValidationLayerSupport();
        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        std::vector<const char*> m_validationLayers = { 
            "VK_LAYER_KHRONOS_validation" 
            // ,"VK_LAYER_LUNARG_api_dump" // For all vulkan calls
        };

        void DescriptorSetLayoutCreate();
        void GraphicsPipelineCreate();
        void RenderPassCreate();
        void FramebuffersCreate();
        void CommandPoolCreate();
        void VertexBufferCreate();
        void IndexBufferCreate();
        void UniformBuffersCreate();
        void DescriptorPoolCreate();
        void DescriptorSetsCreate();
        void CommandBufferCreate();
        void CommandBufferRecord(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void SyncObjectsCreate();
        void UniformBufferUpdate(u16 frameIndex);
    };
} // namespace Quasar
