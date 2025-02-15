#pragma once

#include <qspch.h>
#include "vk_types.h"

namespace Quasar::Renderer {

//> framedata
struct FrameData {
	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
};

constexpr unsigned int FRAME_OVERLAP = 2;
//< framedata

class Backend {
    public:
	bool _isInitialized{ false };
	int _frameNumber {0};

	VkExtent2D _windowExtent{ 1700 , 900 };

	struct GLFWwindow* _window{ nullptr };

//> inst_init
	VkInstance _instance;// Vulkan library handle
	VkDebugUtilsMessengerEXT _debug_messenger;// Vulkan debug output handle
	VkPhysicalDevice _chosenGPU;// GPU chosen as the default device
	VkDevice _device; // Vulkan device for commands
	VkSurfaceKHR _surface;// Vulkan window surface
//< inst_init

//> queues
	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
//< queues
	
//> swap_init
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;

	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;
//< swap_init

	//initializes everything in the engine
	b8 init();

	//shuts down the engine
	void shutdown();

	//draw loop
	void draw();

	bool stop_rendering{false};
private:

	void init_vulkan();

	void init_swapchain();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();

	void init_commands();

	void init_sync_structures();
};
}