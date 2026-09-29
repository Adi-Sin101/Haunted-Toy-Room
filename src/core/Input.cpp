#include "Input.h"

namespace {
Input* inputFrom(GLFWwindow* w)
{
	return static_cast<Input*>(glfwGetWindowUserPointer(w));
}
}

void Input::Attach(GLFWwindow* window)
{
	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetWindowFocusCallback(window, FocusCallback);
}

void Input::EndFrame()
{
	prevKeys = keys;
	prevButtons = buttons;
	mouseDelta = glm::vec2(0.0f);
	scroll = 0.0f;
}

void Input::KeyCallback(GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/)
{
	Input* in = inputFrom(w);
	if (!in || !valid(key))
		return;
	if (action == GLFW_PRESS)
		in->keys[key] = true;
	else if (action == GLFW_RELEASE)
		in->keys[key] = false;
}

void Input::MouseButtonCallback(GLFWwindow* w, int button, int action, int /*mods*/)
{
	Input* in = inputFrom(w);
	if (!in || button < 0 || button > GLFW_MOUSE_BUTTON_LAST)
		return;
	in->buttons[button] = (action == GLFW_PRESS);
}

void Input::CursorCallback(GLFWwindow* w, double x, double y)
{
	Input* in = inputFrom(w);
	if (!in)
		return;
	const glm::vec2 p(static_cast<float>(x), static_cast<float>(y));
	if (in->firstMouse) {
		in->mousePos = p;
		in->firstMouse = false;
	}
	in->mouseDelta += p - in->mousePos;
	in->mousePos = p;
}

void Input::ScrollCallback(GLFWwindow* w, double /*dx*/, double dy)
{
	if (Input* in = inputFrom(w))
		in->scroll += static_cast<float>(dy);
}

void Input::FocusCallback(GLFWwindow* w, int focused)
{
	if (Input* in=inputFrom(w)) {
		in->ResetMouseMotion();
		if (!focused) {
			in->keys.fill(false); in->prevKeys.fill(false);
			in->buttons.fill(false); in->prevButtons.fill(false);
			in->scroll=0;
		}
	}
}
