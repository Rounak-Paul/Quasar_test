#include "Input.h"
#include "Application.h"

namespace Quasar
{
	Input* Input::instance = nullptr;
	keyboard_state Input::keyboard_state;
	mouse_state Input::mouse_state;

	b8 Input::Init() {
		assert(!instance);
        instance = new Input();
		glfwSetKeyCallback(QS_MAIN_WINDOW.GetGLFWwindow(), IsKeyPressed);
		glfwSetMouseButtonCallback(QS_MAIN_WINDOW.GetGLFWwindow() ,IsMbtnPressed);
		return true;
	}

	void Input::Shutdown() {
		
	}

    void Input::IsKeyPressed(GLFWwindow* window, int key, int scancode, int action, int mods) {
		keyboard_state.keys[key] = action;
    }

    void Input::IsMbtnPressed(GLFWwindow* window, int button, int action, int mods) {
		mouse_state.buttons[button] = action;
	}

	glm::vec2 Input::GetMousePosition()
	{
		double xpos, ypos;
		glfwGetCursorPos(QS_MAIN_WINDOW.GetGLFWwindow(), &xpos, &ypos);

		return { (f32)xpos, (f32)ypos };
	}

	f32 Input::GetMouseX()
	{
		return GetMousePosition().x;
	}

	f32 Input::GetMouseY()
	{
		return GetMousePosition().y;
	}
} // namespace Quasar