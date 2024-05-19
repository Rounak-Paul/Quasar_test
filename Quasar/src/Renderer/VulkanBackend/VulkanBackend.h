#pragma once

#include <qspch.h>

namespace Quasar::RendererBackend
{
    class Backend {
        public:
        Backend();
        ~Backend() = default;

        b8 Init(String appName, u16 w, u16 h);
        void Shutdown();

        private:
        VkInstance m_instance; // Not an instance for singleton class
        VkAllocationCallbacks* m_allocator = nullptr;
        VkSurfaceKHR m_surface;
        // vulkan_device device;

        u16 m_width, m_height;

        std::vector<const char*> GetRequiredExtensions();
        b8 CheckValidationLayerSupport();
        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        const std::vector<const char*> m_validationLayers = { 
            "VK_LAYER_KHRONOS_validation" 
            ,"VK_LAYER_LUNARG_api_dump" // For all vulkan calls
        };
    };
} // namespace Quasar
