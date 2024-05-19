#include "VulkanBackend.h"

namespace Quasar::RendererBackend
{
    Backend::Backend() {

    }

    b8 Backend::Init(String appName, u16 w, u16 h) {
        m_width = w;
        m_height = h;
#ifdef QS_DEBUG 
        if (!CheckValidationLayerSupport()) {
            QS_CORE_ERROR("validation layers requested, but not available!");
        }
#endif

        VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
        appInfo.pApplicationName = appName.c_str();
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
        appInfo.pEngineName = "Quasar Engine";
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = GetRequiredExtensions();

        #ifdef QS_PLATFORM_APPLE
            createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        #endif
            createInfo.enabledExtensionCount = extensions.size();
            createInfo.ppEnabledExtensionNames = extensions.data();

        #ifdef QS_DEBUG
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();

            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        #else
            createInfo.enabledLayerCount = 0;
            createInfo.ppEnabledLayerNames = 0;
        #endif

        QS_CORE_INFO("Creating Vulkan instance...");
        VkResult result = vkCreateInstance(&createInfo, allocator, &m_instance);
        if (result != VK_SUCCESS) {
            QS_CORE_ERROR("Instance creation failed with VkResult: %d", result)
            return false;
        }

        return true;
    }
    void Backend::Shutdown() {

    }

    b8 Backend::CheckValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : m_validationLayers) {
            bool layerFound = false;

            for (const auto& it : availableLayers) {
                if (strcmp(layerName, it.layerName) == 0) {
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
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> requiredExtensions;
        for(uint32_t i = 0; i < glfwExtensionCount; i++) {
            requiredExtensions.emplace_back(glfwExtensions[i]);
        }
#ifdef QS_PLATFORM_APPLE
        requiredExtensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
#ifdef QS_DEBUG 
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        return requiredExtensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
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
                    QS_CORE_INFO(pCallbackData->pMessage);
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
        createInfo.pfnUserCallback = debug_callback;
        createInfo.pUserData = nullptr;  // Optional
    }
} // namespace Quasar::RendererBackend
