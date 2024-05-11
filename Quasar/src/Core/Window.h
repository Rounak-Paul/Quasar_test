#pragma once
#include <qspch.h>
#include <Defines.h>

#include <string>

namespace Quasar 
{
	class Window
	{
	public:
		Window(u32 w, u32 h, std::string name);
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		inline b8 should_close() { return glfwWindowShouldClose(window); }
		VkExtent2D get_extent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
        void poll_events();
		
		GLFWwindow* get_GLFWwindow() const { return window; }

	private:
		static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
		static void window_focus_callback(GLFWwindow* window, int focused);

		u32 width;
		u32 height;
		b8 framebufferResized = FALSE;

		std::string windowName;
		GLFWwindow* window;
	};
}
