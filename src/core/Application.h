#pragma once

#include <string>

#include "Input.h"

struct GLFWwindow;

// Owns the GLFW window + OpenGL 3.3 core context and runs the main loop:
//
//   while window open:
//       poll events -> measure delta time -> OnUpdate(dt) -> OnRender() -> swap buffers
//
// Derived classes implement the hooks; they never touch window creation.
class Application {
public:
	Application(int width, int height, std::string title);
	virtual ~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void Run();

protected:
	virtual void OnUpdate(float dt) = 0;
	virtual void OnRender() = 0;
	virtual void OnResize(int /*width*/, int /*height*/) {}

	void SetTitle(const std::string& text);
	void Close();

	GLFWwindow* window = nullptr;
	Input input;
	int width = 0;  // framebuffer size in pixels
	int height = 0;
	float fps = 0.0f;
	double time = 0.0; // seconds since start

private:
	std::string title;
};
