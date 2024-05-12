#include "Window.h"
#include <qspch.h>

namespace Quasar
{
	Window::Window(u32 w, u32 h, std::string name) : width{w}, height{h}, windowName{name} 
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		if (width <= 0 || height <= 0) {
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			width = mode->width; height = mode->height;
		}
		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
		glfwSetWindowFocusCallback(window, window_focus_callback);
	}

	Window::~Window()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Window::framebuffer_resize_callback(GLFWwindow* window, int width, int height)
	{
		
	}

	// GLFW window focus callback
	void Window::window_focus_callback(GLFWwindow* window, int focused) {
		
	}
}