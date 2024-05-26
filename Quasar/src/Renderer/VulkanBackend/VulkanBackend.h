#pragma once

#include <qspch.h>
#include "VulkanTypes.inl"

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanShaderUtil.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"

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

        void DrawFrame(f32 dt);
        void Resize();

        b8 framebufferResized = false;

        std::vector<Vertex> vertices;
        std::vector<u32> indices;

#ifdef QS_PLATFORM_WINDOWS
        const std::string MODEL_PATH = "../../Assets/models/viking_room.obj";
        const std::string TEXTURE_PATH = "../../Assets/textures/viking_room.png";
#else
        const std::string MODEL_PATH = "../Assets/models/viking_room.obj";
        const std::string TEXTURE_PATH = "../Assets/textures/viking_room.png";
#endif

        VulkanTexture textureImage;

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
        void CommandPoolCreate();
        void DepthResourcesCreate();
        void FramebuffersCreate();
        void TextureImageCreate();
        void TextureImageViewCreate();
        void TextureSamplerCreate();
        void ModelLoad();
        void VertexBufferCreate();
        void IndexBufferCreate();
        void UniformBuffersCreate();
        void DescriptorPoolCreate();
        void DescriptorSetsCreate();
        void CommandBufferCreate();
        void CommandBufferRecord(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void SyncObjectsCreate();
        void UniformBufferUpdate(u16 frameIndex);

        void ImageCreate(u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void ImageLayoutTransition(VulkanContext* context, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels);
        void CopyBufferToImage(VulkanContext* context, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat FindDepthFormat();
        bool HasStencilComponent(VkFormat format);
        void MipmapsGenerate(VkImage image, VkFormat imageFormat, i32 texWidth, i32 texHeight, u32 mipLevels);
    };
} // namespace Quasar
