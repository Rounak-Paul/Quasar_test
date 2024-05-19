#include "VulkanDevice.h"

namespace Quasar::RendererBackend
{
#define VK_CHECK(expr)                  \
{                                      \
    assert(expr == VK_SUCCESS);        \
} 

typedef struct VulkanPhysicalDeviceRequirements {
    b8 graphics;
    b8 present;
    b8 compute;
    b8 transfer;
    
    std::vector<const char*> deviceExtensionNames;
    b8 samplerAnisotropy;
    b8 discreteGPU;
} VulkanPhysicalDeviceRequirements;

typedef struct VulkanPhysicalDeviceQueueFamilyInfo {
    u32 graphicsFamilyIndex;
    u32 presentFamilyIndex;
    u32 computeFamilyIndex;
    u32 transferFamilyIndex;
} VulkanPhysicalDeviceQueueFamilyInfo;

b8 PhysicalDeviceMeetsRequirements(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const VulkanPhysicalDeviceRequirements* requirements,
    VulkanPhysicalDeviceQueueFamilyInfo* outQueueFamilyInfo,
    VulkanSwapchainSupportInfo* outSwapchainSupport);

void QuerySwapchainSupport(
            VkPhysicalDevice physical_device,
            VkSurfaceKHR surface,
            VulkanSwapchainSupportInfo* outSupportInfo);

VulkanDevice::VulkanDevice(const VkInstance& vkInstance, const VkSurfaceKHR& vkSurface, VkAllocationCallbacks* allocator) : m_allocator{allocator} {
    if (!SelectPhysicalDevice(vkInstance, vkSurface)) {
        QS_CORE_FATAL("No device with Vulkan support found!")
    }

    // NOTE: Do not create additional queues for shared indices.
    b8 presentSharesGraphicsQueue = m_graphicsQueueIndex == m_presentQueueIndex;
    b8 transferSharesGraphicsQueue = m_graphicsQueueIndex == m_transferQueueIndex;
    b8 presentSharesTransferQueue = m_presentQueueIndex == m_transferQueueIndex;
    
    std::vector<u32> indices;
    indices.push_back(m_graphicsQueueIndex);

    if (!presentSharesGraphicsQueue && std::find(indices.begin(), indices.end(), m_presentQueueIndex) == indices.end()) {
        indices.push_back(m_presentQueueIndex);
    }

    if (!transferSharesGraphicsQueue && std::find(indices.begin(), indices.end(), m_transferQueueIndex) == indices.end()) {
        indices.push_back(m_transferQueueIndex);
    }

    if (!presentSharesTransferQueue && std::find(indices.begin(), indices.end(), m_transferQueueIndex) == indices.end()) {
        indices.push_back(m_transferQueueIndex);
    }

    u32 index_count = indices.size();

    VkQueueFamilyProperties props[32];
    u32 propCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &propCount, 0);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &propCount, props);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos(index_count);
    for (u32 i = 0; i < index_count; ++i) {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = indices[i];
        queue_create_infos[i].queueCount = 1;
        // if (present_shares_graphics_queue && indices[i] == m_graphics_queue_index) {
        //     if (props[m_present_queue_index].queueCount > 1) {
        //         queue_create_infos[i].queueCount = 2;
        //     }
        // }
        queue_create_infos[i].flags = 0;
        queue_create_infos[i].pNext = 0;
        f32 queue_priority = 1.0f;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
    }

    // Request device features.
    // TODO: should be config driven
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;  // Request anistrophy

    VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.pEnabledFeatures = &deviceFeatures;

    // std::vector<f32> queue_priority{1.0f, 1.0f};
    // device_create_info.pQueueCreateInfos->pQueuePriorities = queue_priority.data();
    
    std::vector<const char*> extensionNames = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#ifdef QS_PLATFORM_APPLE
    extensionNames.push_back("VK_KHR_portability_subset");
#endif
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
    device_create_info.ppEnabledExtensionNames = extensionNames.data();

    // Deprecated and ignored, so pass nothing.
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;

    // Create the 
    VK_CHECK(vkCreateDevice(
        m_physicalDevice,
        &device_create_info,
        m_allocator,
        &m_logicalDevice));

    QS_CORE_INFO("Logical device created.");

    // Get queues.
    vkGetDeviceQueue(
        m_logicalDevice,
        m_graphicsQueueIndex,
        0,
        &m_graphicsQueue);

    vkGetDeviceQueue(
        m_logicalDevice,
        m_presentQueueIndex,
        0,
        &m_presentQueue);

    vkGetDeviceQueue(
        m_logicalDevice,
        m_transferQueueIndex,
        0,
        &m_transferQueue);
    QS_CORE_DEBUG("Queues obtained.");

    // Create command pool for graphics queue.
    VkCommandPoolCreateInfo poolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolCreateInfo.queueFamilyIndex = m_graphicsQueueIndex;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(
        m_logicalDevice,
        &poolCreateInfo,
        m_allocator,
        &m_graphicsCommandPool));
    QS_CORE_DEBUG("Graphics command pool created.");
}

void VulkanDevice::Destroy() {
    // Unset queues
    m_graphicsQueue = 0;
    m_presentQueue = 0;
    m_transferQueue = 0;

    QS_CORE_DEBUG("Destroying command pools...");
    vkDestroyCommandPool(
        m_logicalDevice,
        m_graphicsCommandPool,
        m_allocator);

    // Destroy logical device
    QS_CORE_DEBUG("Destroying logical device");
    if (m_logicalDevice) {
        vkDestroyDevice(m_logicalDevice, m_allocator);
        m_logicalDevice = 0;
    }

    // Physical devices are not destroyed.
    QS_CORE_DEBUG("Releasing physical device resources...");
    m_physicalDevice = 0;

    if (!m_swapchainSupport.formats.empty()) {
        m_swapchainSupport.formats.clear();
        m_swapchainSupport.formatCount = 0;
    }

    if (!m_swapchainSupport.presentModes.empty()) {
        m_swapchainSupport.presentModes.clear();
        m_swapchainSupport.presentModeCount = 0;
    }

    m_swapchainSupport.capabilities = {};

    m_graphicsQueueIndex = -1;
    m_presentQueueIndex = -1;
    m_transferQueueIndex = -1;
}

b8 VulkanDevice::SelectPhysicalDevice(const VkInstance& vkInstance, const VkSurfaceKHR& vkSurface) {
    uint32_t physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(vkInstance, &physical_device_count, nullptr));
    if (physical_device_count == 0) {
        QS_CORE_FATAL("No devices which support Vulkan were found.");
        return false;
    }

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    physical_devices.resize(physical_device_count);
    VK_CHECK(vkEnumeratePhysicalDevices(vkInstance, &physical_device_count, physical_devices.data()));
    for (u32 i = 0; i < physical_device_count; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

        // Check if device supports local/host visible combo
        b8 supportsDeviceLocalHostVisible = false;
        for (u32 i = 0; i < memory.memoryTypeCount; ++i) {
            // Check each memory type to see if its bit is set to 1.
            if (
                ((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
                ((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)) {
                supportsDeviceLocalHostVisible = true;
                break;
            }
        }

        // TODO: These requirements should probably be driven by engine
        // configuration.
        VulkanPhysicalDeviceRequirements requirements = {};
        requirements.graphics = true;
        requirements.present = true;
        requirements.transfer = true;
        // NOTE: Enable this if compute will be required.
        // requirements.compute = TRUE;
        requirements.samplerAnisotropy = true;
        requirements.discreteGPU = true;
        requirements.deviceExtensionNames = {};
        requirements.deviceExtensionNames.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef QS_PLATFORM_APPLE
        requirements.discreteGPU = false;
        requirements.deviceExtensionNames.emplace_back("VK_KHR_portability_subset");
#endif

        VulkanPhysicalDeviceQueueFamilyInfo queueInfo = {};
        b8 result = PhysicalDeviceMeetsRequirements(
            physical_devices[i],
            vkSurface,
            &properties,
            &features,
            &requirements,
            &queueInfo,
            &m_swapchainSupport);

        if (result) {
            QS_CORE_DEBUG("Selected device: '%s'.", properties.deviceName);
            // GPU type, etc.
            switch (properties.deviceType) {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    QS_CORE_DEBUG("GPU type is Unknown.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    QS_CORE_DEBUG("GPU type is Integrated.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    QS_CORE_DEBUG("GPU type is Descrete.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    QS_CORE_DEBUG("GPU type is Virtual.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    QS_CORE_DEBUG("GPU type is CPU.");
                    break;
            }

            QS_CORE_DEBUG(
                "GPU Driver version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));

            // Vulkan API version.
            QS_CORE_DEBUG(
                "Vulkan API version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

            // Memory information
            for (u32 j = 0; j < memory.memoryHeapCount; ++j) {
                f32 memory_size_gib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    QS_CORE_DEBUG("Local GPU memory: %.2f GiB", memory_size_gib);
                } else {
                    QS_CORE_DEBUG("Shared System memory: %.2f GiB", memory_size_gib);
                }
            }

            m_physicalDevice = physical_devices[i];
            m_graphicsQueueIndex = queueInfo.graphicsFamilyIndex;
            m_presentQueueIndex = queueInfo.presentFamilyIndex;
            m_transferQueueIndex = queueInfo.transferFamilyIndex;
            // NOTE: set compute index here if needed.

            // Keep a copy of properties, features and memory info for later use.
            m_properties = properties;
            m_features = features;
            m_memory = memory;
            m_supportsDeviceLocalHostVisible = supportsDeviceLocalHostVisible;
            break;
        }
    }

    // Ensure a device was selected
    if (!m_physicalDevice) {
        QS_CORE_ERROR("No physical devices were found which meet the requirements.");
        return false;
    }

    physical_devices.clear();
    QS_CORE_DEBUG("Physical device selected.");
    return true;
}

b8 PhysicalDeviceMeetsRequirements(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const VulkanPhysicalDeviceRequirements* requirements,
    VulkanPhysicalDeviceQueueFamilyInfo* outQueueFamilyInfo,
    VulkanSwapchainSupportInfo* outSwapchainSupport) {
    
    // Evaluate device properties to determine if it meets the needs of our applcation.
    outQueueFamilyInfo->graphicsFamilyIndex = -1;
    outQueueFamilyInfo->presentFamilyIndex = -1;
    outQueueFamilyInfo->computeFamilyIndex = -1;
    outQueueFamilyInfo->transferFamilyIndex = -1;

    // Discrete GPU?
    if (requirements->discreteGPU) {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            QS_CORE_INFO("Device is not a discrete GPU, and one is required. Skipping.");
            return false;
        }
    }

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queueFamilies.data());

    // Look at each queue and see what queues it supports
    QS_CORE_INFO("Graphics | Present | Compute | Transfer | Name");
    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queue_family_count; ++i) {
        u8 currentTransferScore = 0;

        // Graphics queue?
        if (outQueueFamilyInfo->graphicsFamilyIndex == -1 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            outQueueFamilyInfo->graphicsFamilyIndex = i;
            ++currentTransferScore;

            // If also a present queue, this prioritizes grouping of the 2.
            VkBool32 supports_present = VK_FALSE;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
            if (supports_present) {
                outQueueFamilyInfo->presentFamilyIndex = i;
                ++currentTransferScore;
            }
        }

        // Compute queue?
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            outQueueFamilyInfo->computeFamilyIndex = i;
            ++currentTransferScore;
        }

        // Transfer queue?
        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            // Take the index if it is the current lowest. This increases the
            // liklihood that it is a dedicated transfer queue.
            if (currentTransferScore <= min_transfer_score) {
                min_transfer_score = currentTransferScore;
                outQueueFamilyInfo->transferFamilyIndex = i;
            }
        }
    }

        // If a present queue hasn't been found, iterate again and take the first one.
        // This should only happen if there is a queue that supports graphics but NOT
        // present.
        if (outQueueFamilyInfo->presentFamilyIndex == -1) {
            for (u32 i = 0; i < queue_family_count; ++i) {
                VkBool32 supports_present = VK_FALSE;
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
                if (supports_present) {
                    outQueueFamilyInfo->presentFamilyIndex = i;

                    // If they differ, bleat about it and move on. This is just here for troubleshooting
                    // purposes.
                    if (outQueueFamilyInfo->presentFamilyIndex != outQueueFamilyInfo->graphicsFamilyIndex) {
                        QS_CORE_WARN("Warning: Different queue index used for present vs graphics: %u.", i);
                    }
                    break;
                }
            }
    }

    // Print out some info about the device
    QS_CORE_INFO("       %d |       %d |       %d |        %d | %s",
        outQueueFamilyInfo->graphicsFamilyIndex != -1,
        outQueueFamilyInfo->presentFamilyIndex != -1,
        outQueueFamilyInfo->computeFamilyIndex != -1,
        outQueueFamilyInfo->transferFamilyIndex != -1,
        properties->deviceName);

    if (
        (!requirements->graphics || (requirements->graphics && outQueueFamilyInfo->graphicsFamilyIndex != -1)) &&
        (!requirements->present || (requirements->present && outQueueFamilyInfo->presentFamilyIndex != -1)) &&
        (!requirements->compute || (requirements->compute && outQueueFamilyInfo->computeFamilyIndex != -1)) &&
        (!requirements->transfer || (requirements->transfer && outQueueFamilyInfo->transferFamilyIndex != -1))) {
        QS_CORE_INFO("Device meets queue requirements.");
        QS_CORE_DEBUG("Graphics Family Index: %i", outQueueFamilyInfo->graphicsFamilyIndex);
        QS_CORE_DEBUG("Present Family Index:  %i", outQueueFamilyInfo->presentFamilyIndex);
        QS_CORE_DEBUG("Transfer Family Index: %i", outQueueFamilyInfo->transferFamilyIndex);
        QS_CORE_DEBUG("Compute Family Index:  %i", outQueueFamilyInfo->computeFamilyIndex);

        // Query swapchain support.
        QuerySwapchainSupport(
            device,
            surface,
            outSwapchainSupport);

        if (outSwapchainSupport->formatCount < 1 || outSwapchainSupport->presentModeCount < 1) {
            if (!outSwapchainSupport->formats.empty()) {
                outSwapchainSupport->formats.clear();
            }
            if (!outSwapchainSupport->presentModes.empty()) {
                outSwapchainSupport->presentModes.clear();
            }
            QS_CORE_INFO("Required swapchain support not present, skipping ");
            return false;
        }

        // Device extensions.
        if (!requirements->deviceExtensionNames.empty()) {
            u32 available_extension_count = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(
                device,
                nullptr,
                &available_extension_count,
                nullptr));
            std::vector<VkExtensionProperties> availableExtensions(available_extension_count);
            if (available_extension_count != 0) {
                VK_CHECK(vkEnumerateDeviceExtensionProperties(
                    device,
                    0,
                    &available_extension_count,
                    availableExtensions.data()));

                u32 required_extension_count = requirements->deviceExtensionNames.size();
                for (u32 i = 0; i < required_extension_count; ++i) {
                    b8 found = false;
                    for (u32 j = 0; j < available_extension_count; ++j) {
                        if (strcmp(requirements->deviceExtensionNames[i], availableExtensions[j].extensionName)) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        QS_CORE_INFO("Required extension not found: '%s', skipping ", requirements->deviceExtensionNames[i]);
                        return false;
                    }
                }
            }
        }

        // Sampler anisotropy
        if (requirements->samplerAnisotropy && !features->samplerAnisotropy) {
            QS_CORE_INFO("Device does not support samplerAnisotropy, skipping.");
            return false;
        }

        // Device meets all requirements.
        return true;
    }

    return false;
}

void QuerySwapchainSupport(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    VulkanSwapchainSupportInfo* outSupportInfo) {
    // Surface capabilities
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physical_device,
        surface,
        &outSupportInfo->capabilities));

    // Surface formats
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device,
        surface,
        &outSupportInfo->formatCount,
        nullptr));

    if (outSupportInfo->formatCount != 0) {
        if (!outSupportInfo->formats.empty()) {
            outSupportInfo->formats.clear();
        }
        outSupportInfo->formats.resize(outSupportInfo->formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device,
            surface,
            &outSupportInfo->formatCount,
            outSupportInfo->formats.data()));
    }

    // Present modes
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device,
        surface,
        &outSupportInfo->presentModeCount,
        nullptr));
    if (outSupportInfo->presentModeCount != 0) {
        if (outSupportInfo->presentModes.empty()) {
            outSupportInfo->presentModes.clear();
            outSupportInfo->presentModes.resize(outSupportInfo->presentModeCount);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_device,
            surface,
            &outSupportInfo->presentModeCount,
            outSupportInfo->presentModes.data()));
    }
}

b8 VulkanDevice::DetectDepthFormat() {
    // Format candidates
    const u64 candidate_count = 3;
    VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT};

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u64 i = 0; i < candidate_count; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            m_depthFormat = candidates[i];
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            m_depthFormat = candidates[i];
            return true;
        }
    }

    return false;
}

} // namespace Quasar::RendererBackend
