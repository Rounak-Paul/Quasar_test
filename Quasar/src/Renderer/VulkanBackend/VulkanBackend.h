#pragma once

#include <qspch.h>

namespace Quasar::RendererBackend
{
    class Backend {
        public:
        Backend();
        ~Backend() = default;

        b8 init(String app_name, u16 w, u16 h);
        void shutdown();

        private:
        VkInstance instance; // Not an instance for singleton class
        VkAllocationCallbacks* allocator = nullptr;

        u16 width, height;

        std::vector<const char*> get_required_extensions();
        b8 check_validation_layer_support();
        void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        const std::vector<const char*> validation_layers = { 
            "VK_LAYER_KHRONOS_validation" 
            ,"VK_LAYER_LUNARG_api_dump" // For all vulkan calls
        };
    };
} // namespace Quasar
