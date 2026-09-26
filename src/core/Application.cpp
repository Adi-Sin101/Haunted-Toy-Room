#include "Application.h"

#include <algorithm>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

Application::Application(int w, int h, std::string windowTitle)
	: width(w), height(h), title(std::move(windowTitle))
{
	if (!glfwInit())
		throw std::runtime_error("glfwInit failed");

	// Request an OpenGL 3.3 Core Profile context (no deprecated fixed-function pipeline).
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4); // 4x multisample anti-aliasing

	window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window (OpenGL 3.3 required)");
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // v-sync

	// Load all OpenGL function pointers from the driver. No gl* call may happen before this.
	if (!gladLoadGL()) {
		glfwDestroyWindow(window);
		glfwTerminate();
		throw std::runtime_error("gladLoadGL failed");
	}

	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	input.Attach(window);
}

Application::~Application()
{
	if (window)
		glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::Run()
{
	double previous = glfwGetTime();
	double fpsTimer = 0.0;
	int frames = 0;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		const double now = glfwGetTime();
		// Delta time makes every motion frame-rate independent; clamp avoids huge jumps after a stall.
		const float dt = static_cast<float>(std::min(now - previous, 0.1));
		previous = now;
		time = now;

		++frames;
		fpsTimer += dt;
		if (fpsTimer >= 0.5) {
			fps = static_cast<float>(frames / fpsTimer);
			frames = 0;
			fpsTimer = 0.0;
		}

		int fbw = 0, fbh = 0;
		glfwGetFramebufferSize(window, &fbw, &fbh);
		if (fbw > 0 && fbh > 0 && (fbw != width || fbh != height)) {
			width = fbw;
			height = fbh;
			glViewport(0, 0, width, height);
			OnResize(width, height);
		}

		OnUpdate(dt);
		if (width > 0 && height > 0)
			OnRender();
		input.EndFrame();

		glfwSwapBuffers(window);
	}
}

void Application::SetTitle(const std::string& text)
{
	glfwSetWindowTitle(window, text.c_str());
}

void Application::Close()
{
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}
