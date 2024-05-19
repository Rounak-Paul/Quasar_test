#pragma once

#include <qspch.h>

#include "VulkanDevice.h"

namespace Quasar::RendererBackend
{
    class Backend {
        public:
        Backend();
        ~Backend() = default;

        b8 Init(String appName, u16 w, u16 h);
        void Shutdown();

        private:
        VkInstance m_vkInstance;
        VkAllocationCallbacks* m_allocator = nullptr;
        VkSurfaceKHR m_vkSurface;
        VulkanDevice* device = nullptr;
        // vulkan_device device;

        u16 m_width, m_height;

        private:
        std::vector<const char*> GetRequiredExtensions();
        b8 CheckValidationLayerSupport();
        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        std::vector<const char*> m_validationLayers = { 
            "VK_LAYER_KHRONOS_validation" 
            // ,"VK_LAYER_LUNARG_api_dump" // For all vulkan calls
        };
    };
} // namespace Quasar
