#include "VulkanBackend.h"
#include <Core/Application.h>
#include <Core/Filesystem.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "VulkanCreateInfo.h"
#include "VulkanPipeline.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Quasar::RendererBackend
{
    Backend::Backend() {}

    b8 Backend::Init(String appName, u16 w, u16 h) {
        context = new VulkanContext();
        context->width = w;
        context->height = h;

        _InitVulkan(appName);

        // Swapchain
        QS_CORE_DEBUG("Creating Swapchain")
        if (!VulkanSwapchainCreate(context, &context->swapchain)) {
            QS_CORE_FATAL("Failed to create Swapchain!")
            return false;
        }

        // Render Pass
        QS_CORE_DEBUG("Creating Render Pass")
        RenderPassCreate();

        // Descriptor
        QS_CORE_DEBUG("Creating Descriptor Set Layout")
        DescriptorSetLayoutCreate();

        // Graphics Pipeline
        QS_CORE_DEBUG("Creating Graphics pipeline")
        VulkanGraphicsPipelineCreate(context);

        QS_CORE_DEBUG("Creating Command Pool")
        CommandPoolCreate();

        QS_CORE_DEBUG("Creating Color Resource")
        ColorResourcesCreate();

        QS_CORE_DEBUG("Creating Depth Resource")
        DepthResourcesCreate();

        QS_CORE_DEBUG("Creating Swapchain Frame Buffers")
        FramebuffersCreate();

        QS_CORE_DEBUG("Creating Texture Image")
        TextureImageCreate();

        QS_CORE_DEBUG("Creating Texture Image View")
        TextureImageViewCreate();

        QS_CORE_DEBUG("Creating Texture Sampler")
        TextureSamplerCreate();

        QS_CORE_DEBUG("Loading Model")
        ModelLoad();

        QS_CORE_DEBUG("Creating Vertex Buffer")
        VertexBufferCreate();

        QS_CORE_DEBUG("Creating Index Buffer")
        IndexBufferCreate();

        QS_CORE_DEBUG("Creating Uniform Buffer")
        UniformBuffersCreate();

        QS_CORE_DEBUG("Creating Descriptor Pool")
        DescriptorPoolCreate();

        QS_CORE_DEBUG("Creating Descriptor Sets")
        DescriptorSetsCreate();

        // Frame Buffers
        QS_CORE_DEBUG("Creating Command Buffer")
        CommandBufferCreate();

        // Frame Buffers
        QS_CORE_DEBUG("Creating Sync Objects")
        SyncObjectsCreate();

        return true;
    }

    void Backend::Shutdown() {
        vkDeviceWaitIdle(context->device.logicalDevice);

        vkDestroyDescriptorPool(context->device.logicalDevice, context->descriptorPool, nullptr);

        vkDestroySampler(context->device.logicalDevice, textureImage.sampler, nullptr);
        vkDestroyImageView(context->device.logicalDevice, textureImage.texture.view, nullptr);
        vkDestroyImage(context->device.logicalDevice, textureImage.texture.handle, nullptr);
        vkFreeMemory(context->device.logicalDevice, textureImage.texture.memory, nullptr);

        // TODO: move this inside swapchain
        vkDestroyImageView(context->device.logicalDevice, context->swapchain.colorAttachment.view, nullptr);
        vkDestroyImage(context->device.logicalDevice, context->swapchain.colorAttachment.handle, nullptr);
        vkFreeMemory(context->device.logicalDevice, context->swapchain.colorAttachment.memory, nullptr);

        vkDestroyImageView(context->device.logicalDevice, context->swapchain.depthAttachment.view, nullptr);
        vkDestroyImage(context->device.logicalDevice, context->swapchain.depthAttachment.handle, nullptr);
        vkFreeMemory(context->device.logicalDevice, context->swapchain.depthAttachment.memory, nullptr);
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(context->device.logicalDevice, context->renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(context->device.logicalDevice, context->imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(context->device.logicalDevice, context->inFlightFences[i], nullptr);
        }
        if (context->commandPool != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Command Pool")
            vkDestroyCommandPool(context->device.logicalDevice, context->commandPool, context->allocator);
        }
        if (!context->swapchainFramebuffers.empty()) {
            QS_CORE_DEBUG("Destroying Frame Buffers");
            for (auto it : context->swapchainFramebuffers) {
                vkDestroyFramebuffer(context->device.logicalDevice, it, context->allocator);
            }
        }
        VulkanPipelineDestroy(context, context->graphicsPipeline);
        if (context->graphicsPipeline.descriptorSetLayout != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Descriptor Set Layout");
            vkDestroyDescriptorSetLayout(context->device.logicalDevice, context->graphicsPipeline.descriptorSetLayout, nullptr);
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(context->device.logicalDevice, context->uniformBuffers[i].buffer, nullptr);
            vkFreeMemory(context->device.logicalDevice, context->uniformBuffers[i].memory, nullptr);
        }
        if (context->renderpass != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Render Pass");
            vkDestroyRenderPass(context->device.logicalDevice, context->renderpass, context->allocator);
        }

        vkDestroyBuffer(context->device.logicalDevice, context->indexBuffer.buffer, nullptr);
        vkFreeMemory(context->device.logicalDevice, context->indexBuffer.memory, nullptr);

        vkDestroyBuffer(context->device.logicalDevice, context->vertexBuffer.buffer, nullptr);
        vkFreeMemory(context->device.logicalDevice, context->vertexBuffer.memory, nullptr);

        if (context->swapchain.handle != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Swapchain");
            VulkanSwapchainDestroy(context, &context->swapchain);
        }
        QS_CORE_DEBUG("Destroying Vulkan device");
        VulkanDeviceDestroy(context, &context->device);
        
        if (context->surface != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Vulkan surface");
            vkDestroySurfaceKHR(context->instance, context->surface, context->allocator);
            context->surface = VK_NULL_HANDLE;
        }
        if (context->instance != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Vulkan instance");
            vkDestroyInstance(context->instance, context->allocator);
            context->instance = VK_NULL_HANDLE;
        }
    }

    void Backend::Resize() {
        if (QS_APP_STATE.suspended) {
            return;
        }
        QS_CORE_TRACE("Backend resizing...")
        // Requery support
        VulkanDeviceQuerySwapchainSupport(
            context->device.physicalDevice,
            context->surface,
            &context->device.swapchainSupport);
        VulkanDeviceDetectDepthFormat(&context->device);
        vkDeviceWaitIdle(context->device.logicalDevice);
        for (size_t i = 0; i < context->swapchainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(context->device.logicalDevice, context->swapchainFramebuffers[i], nullptr);
        }
        vkDestroyImageView(context->device.logicalDevice, context->swapchain.colorAttachment.view, nullptr);
        vkDestroyImage(context->device.logicalDevice, context->swapchain.colorAttachment.handle, nullptr);
        vkFreeMemory(context->device.logicalDevice, context->swapchain.colorAttachment.memory, nullptr);
        vkDestroyImageView(context->device.logicalDevice, context->swapchain.depthAttachment.view, nullptr);
        vkDestroyImage(context->device.logicalDevice, context->swapchain.depthAttachment.handle, nullptr);
        vkFreeMemory(context->device.logicalDevice, context->swapchain.depthAttachment.memory, nullptr);
        VulkanSwapchainRecreate(context, &context->swapchain);
        ColorResourcesCreate();
        DepthResourcesCreate();
        FramebuffersCreate();
    }

    b8 Backend::CheckValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : m_validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                return false;
            }
        }
        return true;
    }

    std::vector<const char*> Backend::GetRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef QS_PLATFORM_APPLE
        requiredExtensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
#ifdef QS_DEBUG 
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        return requiredExtensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
            switch (messageSeverity)
            {
                default:
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    QS_CORE_ERROR(pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    QS_CORE_WARN(pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                    QS_CORE_DEBUG(pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                    QS_CORE_TRACE(pCallbackData->pMessage);
                    break;
            }
        return VK_FALSE;
    }

    void Backend::PopulateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;
        createInfo.pUserData = nullptr;
    }

    void Backend::RenderPassCreate() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = context->swapchain.swapchainImageFormat;
        colorAttachment.samples = context->device.msaaSamples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = FindDepthFormat();
        depthAttachment.samples = context->device.msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = context->swapchain.swapchainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(context->device.logicalDevice, &renderPassInfo, nullptr, &context->renderpass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void Backend::DescriptorSetLayoutCreate() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(context->device.logicalDevice, &layoutInfo, nullptr, &context->graphicsPipeline.descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void Backend::FramebuffersCreate() {
        context->swapchainFramebuffers.resize(context->swapchain.swapchainImageViews.size());

        for (size_t i = 0; i < context->swapchain.swapchainImageViews.size(); i++) {
            std::array<VkImageView, 3> attachments = {
            context->swapchain.colorAttachment.view,
            context->swapchain.depthAttachment.view,
            context->swapchain.swapchainImageViews[i]
        };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = context->renderpass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = context->swapchain.swapchainExtent.width;
            framebufferInfo.height = context->swapchain.swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(context->device.logicalDevice, &framebufferInfo, nullptr, &context->swapchainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void Backend::CommandPoolCreate() {

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = context->device.graphicsQueueIndex;

        if (vkCreateCommandPool(context->device.logicalDevice, &poolInfo, context->allocator, &context->commandPool) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to create command pool!");
        }
    }

    void Backend::DepthResourcesCreate() {
        VkFormat depthFormat = FindDepthFormat();

        VulkanImageCreate(context, context->swapchain.swapchainExtent.width, context->swapchain.swapchainExtent.height, 1, context->device.msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context->swapchain.depthAttachment);
        context->swapchain.depthAttachment.view = VulkanImageViewCreate(context, context->swapchain.depthAttachment.handle, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    VkFormat Backend::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(context->device.physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat Backend::FindDepthFormat() {
        return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool Backend::HasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void Backend::TextureImageCreate() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        textureImage.mipLevels = static_cast<u32>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            QS_CORE_FATAL("failed to load texture image!");
        }

        VulkanBuffer stagingBuffer;
        VulkanBufferCreate(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

        void* data;
        vkMapMemory(context->device.logicalDevice, stagingBuffer.memory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(context->device.logicalDevice, stagingBuffer.memory);

        stbi_image_free(pixels);

        VulkanImageCreate(
            context, 
            texWidth, 
            texHeight, 
            textureImage.mipLevels, 
            VK_SAMPLE_COUNT_1_BIT, 
            VK_FORMAT_R8G8B8A8_SRGB, 
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            textureImage.texture);

        VulkanImageLayoutTransition(context, textureImage.texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureImage.mipLevels);
            CopyBufferToImage(context, stagingBuffer.buffer, textureImage.texture);
        // ImageLayoutTransition(context, textureImage.texture.handle, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureImage.mipLevels);

        MipmapsGenerate(textureImage.texture.handle, VK_FORMAT_R8G8B8A8_SRGB, textureImage.texture.width, textureImage.texture.height, textureImage.mipLevels);

        vkDestroyBuffer(context->device.logicalDevice, stagingBuffer.buffer, nullptr);
        vkFreeMemory(context->device.logicalDevice, stagingBuffer.memory, nullptr);
    }

    void Backend::ColorResourcesCreate() {
        VkFormat colorFormat = context->swapchain.swapchainImageFormat;

        VulkanImageCreate(context, context->swapchain.swapchainExtent.width, context->swapchain.swapchainExtent.height, 1, context->device.msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context->swapchain.colorAttachment);
        context->swapchain.colorAttachment.view = VulkanImageViewCreate(context, context->swapchain.colorAttachment.handle, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void Backend::MipmapsGenerate(VkImage image, VkFormat imageFormat, i32 texWidth, i32 texHeight, u32 mipLevels) {
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(context->device.physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = SingleUseCommandBegin(context);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        SingleUseCommandEnd(context, commandBuffer);
    }

    void Backend::TextureImageViewCreate() {
        textureImage.texture.view = VulkanImageViewCreate(context, textureImage.texture.handle, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureImage.mipLevels);
    }

    void Backend::TextureSamplerCreate() {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(context->device.physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f; // Optional
        // samplerInfo.minLod = static_cast<f32>(textureImage.mipLevels/2);
        samplerInfo.maxLod = static_cast<f32>(textureImage.mipLevels);
        samplerInfo.mipLodBias = 0.0f; // Optional

        if (vkCreateSampler(context->device.logicalDevice, &samplerInfo, nullptr, &textureImage.sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void Backend::ModelLoad() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            QS_CORE_ERROR((warn + err).c_str());
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void Backend::VertexBufferCreate() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VulkanBuffer stagingBuffer;
        VulkanBufferCreate(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

        void* data;
        vkMapMemory(context->device.logicalDevice, stagingBuffer.memory, 0, bufferSize, 0, &data);
            memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(context->device.logicalDevice, stagingBuffer.memory);

        VulkanBufferCreate(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context->vertexBuffer);

        VulkanBufferCopy(context, stagingBuffer.buffer, context->vertexBuffer.buffer, bufferSize);

        vkDestroyBuffer(context->device.logicalDevice, stagingBuffer.buffer, nullptr);
        vkFreeMemory(context->device.logicalDevice, stagingBuffer.memory, nullptr);
    }

    void Backend::IndexBufferCreate() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VulkanBuffer stagingBuffer;
        VulkanBufferCreate(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

        void* data;
        vkMapMemory(context->device.logicalDevice, stagingBuffer.memory, 0, bufferSize, 0, &data);
            memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(context->device.logicalDevice, stagingBuffer.memory);

        VulkanBufferCreate(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context->indexBuffer);

        VulkanBufferCopy(context, stagingBuffer.buffer, context->indexBuffer.buffer, bufferSize);

        vkDestroyBuffer(context->device.logicalDevice, stagingBuffer.buffer, nullptr);
        vkFreeMemory(context->device.logicalDevice, stagingBuffer.memory, nullptr);
    }

    void Backend::UniformBuffersCreate() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        context->uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        context->uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VulkanBufferCreate(context, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, context->uniformBuffers[i]);

            vkMapMemory(context->device.logicalDevice, context->uniformBuffers[i].memory, 0, bufferSize, 0, &context->uniformBuffersMapped[i]);
        }
    }

    void Backend::DescriptorPoolCreate() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(context->device.logicalDevice, &poolInfo, nullptr, &context->descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void Backend::DescriptorSetsCreate() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, context->graphicsPipeline.descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = context->descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        context->descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(context->device.logicalDevice, &allocInfo, context->descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = context->uniformBuffers[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImage.texture.view;
            imageInfo.sampler = textureImage.sampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = context->descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = context->descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(context->device.logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void Backend::CommandBufferCreate() {
        context->commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = context->commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) context->commandBuffers.size();

        if (vkAllocateCommandBuffers(context->device.logicalDevice, &allocInfo, context->commandBuffers.data()) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to allocate command buffers!");
        }

    }

    void Backend::CommandBufferRecord(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = context->renderpass;
        renderPassInfo.framebuffer = context->swapchainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = context->swapchain.swapchainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphicsPipeline.handle);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float) context->swapchain.swapchainExtent.width;
            viewport.height = (float) context->swapchain.swapchainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = context->swapchain.swapchainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);            

            VkBuffer vertexBuffers[] = {context->vertexBuffer.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffer, context->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphicsPipeline.pipelineLayout, 0, 1, &context->descriptorSets[context->frameIndex], 0, nullptr);

            // vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to record command buffer!");
        }
    }

    void Backend::SyncObjectsCreate() {
        context->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        context->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        context->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(context->device.logicalDevice, &semaphoreInfo, nullptr, &context->imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(context->device.logicalDevice, &semaphoreInfo, nullptr, &context->renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(context->device.logicalDevice, &fenceInfo, nullptr, &context->inFlightFences[i]) != VK_SUCCESS) {
                QS_CORE_FATAL("failed to create synchronization objects for a frame!");
            }
        }
    }

    void Backend::UniformBufferUpdate(u16 currentImage) {
        static f32 clk;
        // clk += context->dt;
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), clk * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), context->swapchain.swapchainExtent.width / (float) context->swapchain.swapchainExtent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        memcpy(context->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void Backend::DrawFrame(f32 dt) {
        context->dt = dt;
        vkWaitForFences(context->device.logicalDevice, 1, &context->inFlightFences[context->frameIndex], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(context->device.logicalDevice, context->swapchain.handle, UINT64_MAX, context->imageAvailableSemaphores[context->frameIndex], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            Resize();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            QS_CORE_FATAL("failed to acquire swap chain image!");
        }

        vkResetFences(context->device.logicalDevice, 1, &context->inFlightFences[context->frameIndex]);

        vkResetCommandBuffer(context->commandBuffers[context->frameIndex], /*VkCommandBufferResetFlagBits*/ 0);
        CommandBufferRecord(context->commandBuffers[context->frameIndex], imageIndex);

        UniformBufferUpdate(context->frameIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {context->imageAvailableSemaphores[context->frameIndex]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &context->commandBuffers[context->frameIndex];

        VkSemaphore signalSemaphores[] = {context->renderFinishedSemaphores[context->frameIndex]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(context->device.graphicsQueue, 1, &submitInfo, context->inFlightFences[context->frameIndex]) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {context->swapchain.handle};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(context->device.presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            Resize();
        } else if (result != VK_SUCCESS) {
            QS_CORE_FATAL("failed to present swap chain image!");
        }

        context->frameIndex = (context->frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    b8 Backend::_InitVulkan(String& appName) {
        #ifdef QS_DEBUG 
        if (!CheckValidationLayerSupport()) {
            QS_CORE_ERROR("Validation layers requested, but not available!");
            return false;
        }
#endif
        VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.pApplicationName = appName.c_str();
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
        appInfo.pEngineName = "Quasar Engine";
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = GetRequiredExtensions();

#ifdef QS_PLATFORM_APPLE
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef QS_DEBUG
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
#endif

        // Instance creation
        QS_CORE_DEBUG("Creating Vulkan instance...");
        VkResult result = vkCreateInstance(&createInfo, context->allocator, &context->instance);
        if (result != VK_SUCCESS) {
            QS_CORE_ERROR("Instance creation failed with VkResult: %d", result);
            return false;
        }

        // Surface creation
        QS_CORE_DEBUG("Creating Vulkan surface...");
        GLFWwindow* window = QS_MAIN_WINDOW.GetGLFWwindow();
        result = glfwCreateWindowSurface(context->instance, window, context->allocator, &context->surface);
        if (result != VK_SUCCESS) {
            QS_CORE_ERROR("Failed to create Window Surface, VkResult: %d", result);
            vkDestroyInstance(context->instance, context->allocator);
            context->instance = VK_NULL_HANDLE;
            return false;
        }

        // Device
        QS_CORE_INFO("Device Selection")
        if (!VulkanDeviceCreate(context, &context->device)) {
            QS_CORE_FATAL("Failed to create Device!")
            return false;
        }

        // initialize the memory allocator
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = context->device.physicalDevice;
        allocatorInfo.device = context->device.logicalDevice;
        allocatorInfo.instance = context->instance;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        if (vmaCreateAllocator(&allocatorInfo, &context->vmaAllocator) != VK_SUCCESS) {
            QS_CORE_FATAL("Failed to create Vulkan Allocator!")
            return false;
        }

        return true;
    }

    void Backend::_InitSwapchain() {
        VulkanSwapchainCreate(context, &context->swapchain);

        context->drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        context->drawImage.extent = { context->swapchain.swapchainExtent.width, context->swapchain.swapchainExtent.height };

        VkImageUsageFlags drawImageUsages{};
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        VkImageCreateInfo rimg_info = VulkanImageCreateInfo(context->drawImage.format, drawImageUsages, context->drawImage.extent);

        //for the draw image, we want to allocate it from gpu local memory
        VmaAllocationCreateInfo rimg_allocinfo = {};
        rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        //allocate and create the image
        vmaCreateImage(context->vmaAllocator, &rimg_info, &rimg_allocinfo, &context->drawImage.image, &context->drawImage.allocation, nullptr);

        //build a image-view for the draw image to use for rendering
        VkImageViewCreateInfo rview_info = VulkanImageViewCreateInfo(context->drawImage.format, context->drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(context->device.logicalDevice, &rview_info, nullptr, &context->drawImage.view));

        //create a depth image too
        //hardcoding the draw format to 32 bit float
        context->depthImage.format = VK_FORMAT_D32_SFLOAT;
        context->depthImage.extent = context->drawImage.extent;

        VkImageUsageFlags depthImageUsages{};
        depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkImageCreateInfo dimg_info = VulkanImageCreateInfo(context->depthImage.format, depthImageUsages, context->drawImage.extent);

        //allocate and create the image
        vmaCreateImage(context->vmaAllocator, &dimg_info, &rimg_allocinfo, &context->depthImage.image, &context->depthImage.allocation, nullptr);

        //build a image-view for the draw image to use for rendering
        VkImageViewCreateInfo dview_info = VulkanImageViewCreateInfo(context->depthImage.format, context->depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

        VK_CHECK(vkCreateImageView(context->device.logicalDevice, &dview_info, nullptr, &context->depthImage.view));


        //add to deletion queues
        mainDeletionQueue.push_function([=]() {
            vkDestroyImageView(context->device.logicalDevice, context->drawImage.view, nullptr);
            vmaDestroyImage(context->vmaAllocator, context->drawImage.image, context->drawImage.allocation);

            vkDestroyImageView(context->device.logicalDevice, context->depthImage.view, nullptr);
            vmaDestroyImage(context->vmaAllocator, context->depthImage.image, context->depthImage.allocation);
        });
    }

    void Backend::_ResizeSwapchain() {
        vkDeviceWaitIdle(context->device.logicalDevice);
        VulkanSwapchainRecreate(context, &context->swapchain);
        context->resizeRequested = false;
    }

    void Backend::_InitCommands() {
        // create a command pool for commands submitted to the graphics queue.
        // we also want the pool to allow for resetting of individual command buffers
        VkCommandPoolCreateInfo commandPoolInfo = VulkanCommandPoolCreateInfo(context->device.graphicsQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (u8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            VK_CHECK(vkCreateCommandPool(context->device.logicalDevice, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

            // allocate the default command buffer that we will use for rendering
            VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

            VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));

            _mainDeletionQueue.push_function([=]() { vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr); });
        }

        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immCommandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_immCommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immCommandBuffer));

        mainDeletionQueue.push_function([=]() { vkDestroyCommandPool(context->device.logicalDevice, _immCommandPool, nullptr); });
    }
} // namespace Quasar::RendererBackend
