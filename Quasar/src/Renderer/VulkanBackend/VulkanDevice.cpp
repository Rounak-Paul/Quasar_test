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

b8 __PhysicalDeviceMeetsRequirements(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const VulkanPhysicalDeviceRequirements* requirements,
    VulkanPhysicalDeviceQueueFamilyInfo* outQueueFamilyInfo,
    VulkanSwapchainSupportInfo* outSwapchainSupport);

b8 __PhysicalDeviceSelect(VulkanContext* context, b8 discreteGPU, VulkanDevice* outDevice);

b8 VulkanDeviceCreate(VulkanContext* context, VulkanDevice* outDevice) {
    if (!__PhysicalDeviceSelect(context, true, outDevice)) {
        QS_CORE_WARN("No Discrete GPU with Vulkan support found. Defaulting to Integrated GPU.")
        if (!__PhysicalDeviceSelect(context, false, outDevice)) {
            QS_CORE_FATAL("No Device with Vulkan support found")
            return false;
        }
    }

    context->device.msaaSamples = GetMaxUsableSampleCount(context);

    // NOTE: Do not create additional queues for shared indices.
    b8 presentSharesGraphicsQueue = outDevice->graphicsQueueIndex == outDevice->presentQueueIndex;
    b8 transferSharesGraphicsQueue = outDevice->graphicsQueueIndex == outDevice->transferQueueIndex;
    b8 presentSharesTransferQueue = outDevice->presentQueueIndex == outDevice->transferQueueIndex;
    
    std::vector<u32> indices;
    indices.push_back(outDevice->graphicsQueueIndex);

    if (!presentSharesGraphicsQueue && std::find(indices.begin(), indices.end(), outDevice->presentQueueIndex) == indices.end()) {
        indices.push_back(outDevice->presentQueueIndex);
    }

    if (!transferSharesGraphicsQueue && std::find(indices.begin(), indices.end(), outDevice->transferQueueIndex) == indices.end()) {
        indices.push_back(outDevice->transferQueueIndex);
    }

    if (!presentSharesTransferQueue && std::find(indices.begin(), indices.end(), outDevice->transferQueueIndex) == indices.end()) {
        indices.push_back(outDevice->transferQueueIndex);
    }

    u32 indexCount = indices.size();

    VkQueueFamilyProperties props[32];
    u32 propCount;
    vkGetPhysicalDeviceQueueFamilyProperties(outDevice->physicalDevice, &propCount, 0);
    vkGetPhysicalDeviceQueueFamilyProperties(outDevice->physicalDevice, &propCount, props);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(indexCount);
    for (u32 i = 0; i < indexCount; ++i) {
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = indices[i];
        queueCreateInfos[i].queueCount = 1;
        // if (presentSharesGraphicsQueue && indices[i] == graphicsQueueIndex) {
        //     if (props[presentQueueIndex].queueCount > 1) {
        //         queueCreateInfos[i].queueCount = 2;
        //     }
        // }
        queueCreateInfos[i].flags = 0;
        queueCreateInfos[i].pNext = 0;
        f32 queue_priority = 1.0f;
        queueCreateInfos[i].pQueuePriorities = &queue_priority;
    }

    // Request device features.
    // TODO: should be config driven
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;  // Request anistrophy

    VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = indexCount;
    device_create_info.pQueueCreateInfos = queueCreateInfos.data();
    device_create_info.pEnabledFeatures = &deviceFeatures;
    
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
        outDevice->physicalDevice,
        &device_create_info,
        context->allocator,
        &outDevice->logicalDevice));

    QS_CORE_INFO("Logical device created.");

    // Get queues.
    vkGetDeviceQueue(
        outDevice->logicalDevice,
        outDevice->graphicsQueueIndex,
        0,
        &outDevice->graphicsQueue);

    vkGetDeviceQueue(
        outDevice->logicalDevice,
        outDevice->presentQueueIndex,
        0,
        &outDevice->presentQueue);

    vkGetDeviceQueue(
        outDevice->logicalDevice,
        outDevice->transferQueueIndex,
        0,
        &outDevice->transferQueue);
    QS_CORE_DEBUG("Queues obtained.");

    // Create command pool for graphics queue.
    VkCommandPoolCreateInfo poolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolCreateInfo.queueFamilyIndex = outDevice->graphicsQueueIndex;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(
        outDevice->logicalDevice,
        &poolCreateInfo,
        context->allocator,
        &outDevice->graphicsCommandPool));
    QS_CORE_DEBUG("Graphics command pool created.");

    return true;
}

VkSampleCountFlagBits GetMaxUsableSampleCount(VulkanContext* context) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(context->device.physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanDeviceDestroy(VulkanContext* context, VulkanDevice* device) {
    // Unset queues
    device->graphicsQueue = 0;
    device->presentQueue = 0;
    device->transferQueue = 0;

    QS_CORE_DEBUG("Destroying command pools...");
    vkDestroyCommandPool(
        device->logicalDevice,
        device->graphicsCommandPool,
        context->allocator);

    // Destroy logical device
    QS_CORE_DEBUG("Destroying logical device");
    if (device->logicalDevice) {
        vkDestroyDevice(device->logicalDevice, context->allocator);
        device->logicalDevice = 0;
    }

    // Physical devices are not destroyed.
    QS_CORE_DEBUG("Releasing physical device resources...");
    device->physicalDevice = 0;

    if (!device->swapchainSupport.formats.empty()) {
        device->swapchainSupport.formats.clear();
    }

    if (!device->swapchainSupport.presentModes.empty()) {
        device->swapchainSupport.presentModes.clear();
    }

    device->swapchainSupport.capabilities = {};

    device->graphicsQueueIndex = -1;
    device->presentQueueIndex = -1;
    device->transferQueueIndex = -1;
}

b8 __PhysicalDeviceSelect(VulkanContext* context, b8 discreteGPU, VulkanDevice* outDevice) {
    uint32_t physicalDeviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physicalDeviceCount, nullptr));
    if (physicalDeviceCount == 0) {
        QS_CORE_FATAL("No devices which support Vulkan were found.");
        return false;
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    physicalDevices.resize(physicalDeviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physicalDeviceCount, physicalDevices.data()));
    for (u32 i = 0; i < physicalDeviceCount; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physicalDevices[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memory);

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
        requirements.discreteGPU = discreteGPU;
        requirements.deviceExtensionNames = {};
        requirements.deviceExtensionNames.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef QS_PLATFORM_APPLE
        requirements.discreteGPU = false;
        requirements.deviceExtensionNames.emplace_back("VK_KHR_portability_subset");
#endif

        VulkanPhysicalDeviceQueueFamilyInfo queueInfo = {};
        b8 result = __PhysicalDeviceMeetsRequirements(
            physicalDevices[i],
            context->surface,
            &properties,
            &features,
            &requirements,
            &queueInfo,
            &outDevice->swapchainSupport);

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

            outDevice->physicalDevice = physicalDevices[i];
            outDevice->graphicsQueueIndex = queueInfo.graphicsFamilyIndex;
            outDevice->presentQueueIndex = queueInfo.presentFamilyIndex;
            outDevice->transferQueueIndex = queueInfo.transferFamilyIndex;
            // NOTE: set compute index here if needed.

            // Keep a copy of properties, features and memory info for later use.
            properties = properties;
            features = features;
            memory = memory;
            supportsDeviceLocalHostVisible = supportsDeviceLocalHostVisible;
            break;
        }
    }

    // Ensure a device was selected
    if (!outDevice->physicalDevice) {
        return false;
    }

    physicalDevices.clear();
    QS_CORE_DEBUG("Physical device selected.");
    return true;
}

b8 __PhysicalDeviceMeetsRequirements(
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
        QS_CORE_INFO("Discrete GPU found.");
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
        VulkanDeviceQuerySwapchainSupport(
            device,
            surface,
            outSwapchainSupport);

        if (outSwapchainSupport->formats.size() < 1 || outSwapchainSupport->presentModes.size() < 1) {
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

void VulkanDeviceQuerySwapchainSupport(
    VkPhysicalDevice physicalDevice, 
    VkSurfaceKHR surface,
    VulkanSwapchainSupportInfo* outSupportInfo) {
    // Surface capabilities
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice,
        surface,
        &outSupportInfo->capabilities));

    // Surface formats
    u32 formatCount;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface,
        &formatCount,
        nullptr));

    if (formatCount != 0) {
        if (!outSupportInfo->formats.empty()) {
            outSupportInfo->formats.clear();
        }
        outSupportInfo->formats.resize(formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &formatCount,
            outSupportInfo->formats.data()));
    }

    // Present modes
    u32 presentModeCount;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice,
        surface,
        &presentModeCount,
        nullptr));
    if (presentModeCount != 0) {
        if (outSupportInfo->presentModes.empty()) {
            outSupportInfo->presentModes.clear();
            outSupportInfo->presentModes.resize(presentModeCount);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &presentModeCount,
            outSupportInfo->presentModes.data()));
    }
}

b8 VulkanDeviceDetectDepthFormat(VulkanDevice* device) {
    // Format candidates
    const u64 candidate_count = 3;
    VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT};

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u64 i = 0; i < candidate_count; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physicalDevice, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            device->depthFormat = candidates[i];
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            device->depthFormat = candidates[i];
            return true;
        }
    }
    return false;
}

} // namespace Quasar::RendererBackend
