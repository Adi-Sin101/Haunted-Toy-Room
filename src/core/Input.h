#pragma once

#include <array>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// Keyboard / mouse state collected from GLFW callbacks.
//
//   Down(key)     -> key is held this frame           (continuous actions: W/S/A/D movement)
//   Pressed(key)  -> key went down since last frame   (one-shot actions: select, mount, toggles)
//
// Pressed() is the "previous-key-state" edge detection: pressing R once mounts once instead of
// toggling every frame while the key is held.
class Input {
public:
	void Attach(GLFWwindow* window);
	void EndFrame(); // call after the frame's logic: current state becomes previous state

	bool Down(int key) const { return valid(key) && keys[key]; }
	bool Pressed(int key) const { return valid(key) && keys[key] && !prevKeys[key]; }
	bool ShiftDown() const { return Down(GLFW_KEY_LEFT_SHIFT) || Down(GLFW_KEY_RIGHT_SHIFT); }
	bool CtrlDown() const { return Down(GLFW_KEY_LEFT_CONTROL) || Down(GLFW_KEY_RIGHT_CONTROL); }

	bool MouseDown(int button) const { return buttons[button]; }
	bool MousePressed(int button) const { return buttons[button] && !prevButtons[button]; }
	bool MouseReleased(int button) const { return !buttons[button] && prevButtons[button]; }
	glm::vec2 MousePosition() const { return mousePos; }
	glm::vec2 MouseDelta() const { return mouseDelta; }
	float Scroll() const { return scroll; }

private:
	static bool valid(int key) { return key >= 0 && key <= GLFW_KEY_LAST; }

	static void KeyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);
	static void MouseButtonCallback(GLFWwindow* w, int button, int action, int mods);
	static void CursorCallback(GLFWwindow* w, double x, double y);
	static void ScrollCallback(GLFWwindow* w, double dx, double dy);

	std::array<bool, GLFW_KEY_LAST + 1> keys{};
	std::array<bool, GLFW_KEY_LAST + 1> prevKeys{};
	std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> buttons{};
	std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> prevButtons{};
	glm::vec2 mousePos{ 0.0f };
	glm::vec2 mouseDelta{ 0.0f };
	bool firstMouse = true;
	float scroll = 0.0f;
};
