#include "pch.h"
#include "GLCore/Core/Input.h"

#include "GLCore/Core/Application.h"
#include <GLFW/glfw3.h>

namespace GLCore {

	bool Input::IsKeyPressed (KeyCode keycode)
	{
		auto window = static_cast<GLFWwindow *>(Application::Get ().GetWindow ().GetNativeWindow ());
		auto state = glfwGetKey (window, static_cast<uint16_t>(keycode));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed (MouseCode button)
	{
		auto window = static_cast<GLFWwindow *>(Application::Get ().GetWindow ().GetNativeWindow ());
		auto state = glfwGetMouseButton (window, static_cast<uint16_t>(button));
		return state == GLFW_PRESS;
	}

	std::pair<float, float> Input::GetMousePosn ()
	{
		auto window = static_cast<GLFWwindow *>(Application::Get ().GetWindow ().GetNativeWindow ());
		double xpos, ypos;
		glfwGetCursorPos (window, &xpos, &ypos);
		return { (float)xpos, (float)ypos };
	}

	float Input::GetMouseX ()
	{
		auto [x, y] = GetMousePosn ();
		return x;
	}

	float Input::GetMouseY ()
	{
		auto [x, y] = GetMousePosn ();
		return y;
	}
}