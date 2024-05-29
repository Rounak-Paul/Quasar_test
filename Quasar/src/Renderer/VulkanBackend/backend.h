#pragma once

#include <qspch.h>
#include "vk_types.h"

namespace Quasar::Renderer {
class Backend {
public:

	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 800 , 600 };

	static Backend& Get();

	//initializes everything in the engine
	b8 init();

	//shuts down the engine
	void shutdown();

	//draw loop
	void draw();
};
}