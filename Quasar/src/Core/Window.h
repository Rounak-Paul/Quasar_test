#pragma once
#include <qspch.h>

#include <string>

namespace Quasar 
{
	class Window
	{
	public:
		Window(u32 w, u32 h, String name);
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		inline b8 ShouldClose() { return glfwWindowShouldClose(m_window); }
		VkExtent2D GetExtent() { return { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) }; }
        QS_INLINE void PollEvents() {glfwPollEvents();};
		QS_INLINE void WaitEvents() {glfwWaitEvents();};
		
		GLFWwindow* GetGLFWwindow() const { return m_window; }

	private:
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		static void WindowFocusCallback(GLFWwindow* window, int focused);

		u32 m_width;
		u32 m_height;
		b8 m_framebufferResized = false;

		String m_windowName;
		GLFWwindow* m_window;
	};
}
