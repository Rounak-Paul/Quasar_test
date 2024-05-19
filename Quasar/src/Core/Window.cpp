#include "Window.h"
#include <qspch.h>

#include <Core/Event.h>

namespace Quasar
{
	Window::Window(u32 w, u32 h, String name) : m_width{w}, m_height{h}, m_windowName{name} 
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		if (m_width <= 0 || m_height <= 0) {
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			m_width = mode->width; m_height = mode->height;
		}
		m_window = glfwCreateWindow(m_width, m_height, m_windowName.c_str(), nullptr, nullptr);
		
		glfwSetWindowUserPointer(m_window, this);
		glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
		glfwSetWindowFocusCallback(m_window, WindowFocusCallback);
	}

	Window::~Window()
	{
		QS_CORE_INFO("Destroying main window")
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void Window::FramebufferResizeCallback(GLFWwindow* window, int m_width, int m_height)
	{
		auto i_window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		i_window->m_width = m_width;
		i_window->m_height = m_height;
		i_window->m_framebufferResized = true;
		EventContext context;
		context.data.u16[0] = m_width;
		context.data.u16[1] = m_height;
		QS_EVENT.Execute(EVENT_CODE_RESIZED, nullptr, context);
	}

	// GLFW window focus callback
	void Window::WindowFocusCallback(GLFWwindow* window, int focused) {
		
	}
}