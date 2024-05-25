#include "VulkanBackend.h"
#include <Core/Application.h>
#include <Core/Filesystem.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Quasar::RendererBackend
{
    Backend::Backend() {}

    b8 Backend::Init(String appName, u16 w, u16 h) {
        context = new VulkanContext();
        context->width = w;
        context->height = h;
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
        GraphicsPipelineCreate();

        // Frame Buffers
        QS_CORE_DEBUG("Creating Swapchain Frame Buffers")
        FramebuffersCreate();

        // Frame Buffers
        QS_CORE_DEBUG("Creating Command Pool")
        CommandPoolCreate();

        QS_CORE_DEBUG("Creating Texture Image")
        TextureImageCreate();

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

        vkDestroyImage(context->device.logicalDevice, textureImage, nullptr);
        vkFreeMemory(context->device.logicalDevice, textureImageMemory, nullptr);
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(context->device.logicalDevice, context->renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(context->device.logicalDevice, context->imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(context->device.logicalDevice, context->inFlightFences[i], nullptr);
        }
        if (context->commandPool != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Command Pool")
            vkDestroyCommandPool(context->device.logicalDevice, context->commandPool, context->allocator);
        }
        if (!context->swapChainFramebuffers.empty()) {
            QS_CORE_DEBUG("Destroying Frame Buffers");
            for (auto it : context->swapChainFramebuffers) {
                vkDestroyFramebuffer(context->device.logicalDevice, it, context->allocator);
            }
        }
        if (context->graphicsPipeline != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Graphics Pipeline");
            vkDestroyPipeline(context->device.logicalDevice, context->graphicsPipeline, context->allocator);
        }
        if (context->descriptorSetLayout != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Descriptor Set Layout");
            vkDestroyDescriptorSetLayout(context->device.logicalDevice, context->descriptorSetLayout, nullptr);
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(context->device.logicalDevice, context->uniformBuffers[i], nullptr);
            vkFreeMemory(context->device.logicalDevice, context->uniformBuffersMemory[i], nullptr);
        }
        if (context->pipelineLayout != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Pipeline Layout");
            vkDestroyPipelineLayout(context->device.logicalDevice, context->pipelineLayout, context->allocator);
        }
        if (context->renderpass != VK_NULL_HANDLE) {
            QS_CORE_DEBUG("Destroying Render Pass");
            vkDestroyRenderPass(context->device.logicalDevice, context->renderpass, context->allocator);
        }

        vkDestroyBuffer(context->device.logicalDevice, context->indexBuffer, nullptr);
        vkFreeMemory(context->device.logicalDevice, context->indexBufferMemory, nullptr);

        vkDestroyBuffer(context->device.logicalDevice, context->vertexBuffer, nullptr);
        vkFreeMemory(context->device.logicalDevice, context->vertexBufferMemory, nullptr);

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
        QS_CORE_TRACE("Backend resizing...")
        // Requery support
        VulkanDeviceQuerySwapchainSupport(
            context->device.physicalDevice,
            context->surface,
            &context->device.swapchainSupport);
        VulkanDeviceDetectDepthFormat(&context->device);
        vkDeviceWaitIdle(context->device.logicalDevice);
        for (size_t i = 0; i < context->swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(context->device.logicalDevice, context->swapChainFramebuffers[i], nullptr);
        }
        VulkanSwapchainRecreate(context, &context->swapchain);
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
        colorAttachment.format = context->swapchain.swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(context->device.logicalDevice, &renderPassInfo, nullptr, &context->renderpass) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to create render pass!");
        }
    }

    void Backend::DescriptorSetLayoutCreate() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(context->device.logicalDevice, &layoutInfo, nullptr, &context->descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void Backend::GraphicsPipelineCreate() {
        Filesystem fs;
#ifdef QS_PLATFORM_WINDOWS
        auto vertShaderCode = fs.ReadBinary("../../Assets/shaders/Builtin.MaterialShader.vert.spv");
        auto fragShaderCode = fs.ReadBinary("../../Assets/shaders/Builtin.MaterialShader.frag.spv");
#else
        auto vertShaderCode = fs.ReadBinary("../Assets/shaders/Builtin.MaterialShader.vert.spv");
        auto fragShaderCode = fs.ReadBinary("../Assets/shaders/Builtin.MaterialShader.frag.spv");
#endif

        VkShaderModule vertShaderModule = ShaderModuleCreate(&context->device ,vertShaderCode);
        VkShaderModule fragShaderModule = ShaderModuleCreate(&context->device, fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &context->descriptorSetLayout;

        if (vkCreatePipelineLayout(context->device.logicalDevice, &pipelineLayoutInfo, nullptr, &context->pipelineLayout) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = context->pipelineLayout;
        pipelineInfo.renderPass = context->renderpass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(context->device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &context->graphicsPipeline) != VK_SUCCESS) {
            QS_CORE_FATAL("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(context->device.logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(context->device.logicalDevice, vertShaderModule, nullptr);
    }

    void Backend::FramebuffersCreate() {
        context->swapChainFramebuffers.resize(context->swapchain.swapChainImageViews.size());

        for (size_t i = 0; i < context->swapchain.swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                context->swapchain.swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = context->renderpass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = context->swapchain.swapChainExtent.width;
            framebufferInfo.height = context->swapchain.swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(context->device.logicalDevice, &framebufferInfo, nullptr, &context->swapChainFramebuffers[i]) != VK_SUCCESS) {
                QS_CORE_FATAL("failed to create framebuffer!");
            }
        }
    }

    void Backend::CommandPoolCreate() {

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = context->device.graphicsQueueIndex;

        if (vkCreateCommandPool(context->device.logicalDevice, &poolInfo, context->allocator, &context->commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void Backend::TextureImageCreate() {
        int texWidth, texHeight, texChannels;
#ifdef QS_PLATFORM_WINDOWS
        stbi_uc* pixels = stbi_load("../../Assets/textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
#else
        stbi_uc* pixels = stbi_load("../Assets/textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
#endif
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VulkanBufferCreate(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(context->device.logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(context->device.logicalDevice, stagingBufferMemory);

        stbi_image_free(pixels);

        ImageCreate(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

        ImageLayoutTransition(context, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            CopyBufferToImage(context, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        ImageLayoutTransition(context, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(context->device.logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(context->device.logicalDevice, stagingBufferMemory, nullptr);
    }

    void Backend::ImageCreate(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(context->device.logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(context->device.logicalDevice, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(context, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(context->device.logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(context->device.logicalDevice, image, imageMemory, 0);
    }

    void Backend::CopyBufferToImage(VulkanContext* context, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = SingleUseCommandBegin(context);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        SingleUseCommandEnd(context, commandBuffer);
    }

    void Backend::ImageLayoutTransition(VulkanContext* context, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = SingleUseCommandBegin(context);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        SingleUseCommandEnd(context, commandBuffer);
    }

    void Backend::VertexBufferCreate() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VulkanBufferCreate(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(context->device.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(context->device.logicalDevice, stagingBufferMemory);

        VulkanBufferCreate(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context->vertexBuffer, context->vertexBufferMemory);

        VulkanBufferCopy(context, stagingBuffer, context->vertexBuffer, bufferSize);

        vkDestroyBuffer(context->device.logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(context->device.logicalDevice, stagingBufferMemory, nullptr);
    }

    void Backend::IndexBufferCreate() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VulkanBufferCreate(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(context->device.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(context->device.logicalDevice, stagingBufferMemory);

        VulkanBufferCreate(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context->indexBuffer, context->indexBufferMemory);

        VulkanBufferCopy(context, stagingBuffer, context->indexBuffer, bufferSize);

        vkDestroyBuffer(context->device.logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(context->device.logicalDevice, stagingBufferMemory, nullptr);
    }

    void Backend::UniformBuffersCreate() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        context->uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        context->uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        context->uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VulkanBufferCreate(context, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, context->uniformBuffers[i], context->uniformBuffersMemory[i]);

            vkMapMemory(context->device.logicalDevice, context->uniformBuffersMemory[i], 0, bufferSize, 0, &context->uniformBuffersMapped[i]);
        }
    }

    void Backend::DescriptorPoolCreate() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(context->device.logicalDevice, &poolInfo, nullptr, &context->descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void Backend::DescriptorSetsCreate() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, context->descriptorSetLayout);
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
            bufferInfo.buffer = context->uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = context->descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(context->device.logicalDevice, 1, &descriptorWrite, 0, nullptr);
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
            throw std::runtime_error("failed to allocate command buffers!");
        }

    }

    void Backend::CommandBufferRecord(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = context->renderpass;
        renderPassInfo.framebuffer = context->swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = context->swapchain.swapChainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphicsPipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float) context->swapchain.swapChainExtent.width;
            viewport.height = (float) context->swapchain.swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = context->swapchain.swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);            

            VkBuffer vertexBuffers[] = {context->vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffer, context->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipelineLayout, 0, 1, &context->descriptorSets[context->frameIndex], 0, nullptr);

            // vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
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
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void Backend::UniformBufferUpdate(u16 currentImage) {
        static f32 clk;
        clk += context->dt;
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), clk * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), context->swapchain.swapChainExtent.width / (float) context->swapchain.swapChainExtent.height, 0.1f, 10.0f);
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
            throw std::runtime_error("failed to acquire swap chain image!");
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
            throw std::runtime_error("failed to submit draw command buffer!");
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
            throw std::runtime_error("failed to present swap chain image!");
        }

        context->frameIndex = (context->frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }
} // namespace Quasar::RendererBackend
