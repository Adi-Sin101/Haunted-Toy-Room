#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl/Shader.h"
#include "gl/VBO.h"
#include "gl/EBO.h"
#include "gl/VAO.h"

using namespace std;
using namespace glm;

const unsigned int width = 800;
const unsigned int height = 800;

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//GLfloat vertices[] = {
	//	-0.5f,		-0.5f * sqrt(3) / 3,										+0.0f, 0.1019f, 0.7372f, 0.6117f, // Lower left, 0
	//	+0.5f,		-0.5f * sqrt(3) / 3,										+0.0f, 0.1803f, 0.8000f, 0.4431f, // Lower right, 1
	//	+0.0f,		+0.5f * sqrt(3) * 2 / 3,									+0.0f, 0.2039f, 0.5960f, 0.8588f, // Upper, 2
	//	-0.5f / 2,	((+0.5f * sqrt(3) * 2 / 3) + (-0.5f * sqrt(3) / 3)) / 2,	+0.0f, 0.6078f, 0.3490f, 0.7137f, // Inner left, 3
	//	+0.5f / 2,	((+0.5f * sqrt(3) * 2 / 3) + (-0.5f * sqrt(3) / 3)) / 2,	+0.0f, 0.2039f, 0.2862f, 0.3686f, // Inner right, 4
	//	+0.0f,		-0.5f * sqrt(3) / 3,										+0.0f, 0.9450f, 0.7686f, 0.0588f  // Inner down, 5
	//};

	//GLuint indices[] = {
	//	3, 0, 5,
	//	4, 5, 1,
	//	2, 3, 4
	//};

	//GLfloat vertices[] = {
	//	// Position                 // Color
	//	// x      y      z          r       g       b

	//	-0.5f, -0.5f,  0.5f,       1.0f,   0.0f,   0.0f,   // 0: Front-left
	//	 0.5f, -0.5f,  0.5f,       0.0f,   1.0f,   0.0f,   // 1: Front-right
	//	 0.5f, -0.5f, -0.5f,       0.0f,   0.0f,   1.0f,   // 2: Back-right
	//	-0.5f, -0.5f, -0.5f,       1.0f,   1.0f,   0.0f,   // 3: Back-left

	//	 0.0f,  0.5f,  0.0f,       1.0f,   0.0f,   1.0f    // 4: Top
	//};

	//GLuint indices[] = {
	//	// Four triangular sides
	//	0, 1, 4,      // Front
	//	1, 2, 4,      // Right
	//	2, 3, 4,      // Back
	//	3, 0, 4,      // Left

	//	// Square base (two triangles)
	//	0, 3, 2,
	//	0, 2, 1
	//};

	GLfloat vertices[] = {
		0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, //0
		1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, //1
		1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, //2
		0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f // 3
	};

	GLuint indices[] = {
		0, 1, 2,
		0, 3, 2
	};

	GLFWwindow* window = glfwCreateWindow(width, height, "Haunted Toy Room", NULL, NULL);
	if (window == NULL) {
		cout << "Failed to create window!" << endl;
		return -1;
	}
	glfwMakeContextCurrent(window);

	gladLoadGL();

	glViewport(0, 0, width, height);

	Shader shaderProgram("shaders/default.vert", "shaders/default.frag");

	VAO VAO1;
	VAO1.Bind();

	VBO VBO1(vertices, sizeof(vertices));
	EBO EBO1(indices, sizeof(indices));

	VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 6 * sizeof(GL_FLOAT), (void*)0);
	VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 6 * sizeof(GL_FLOAT), (void*)(3 * sizeof(GL_FLOAT)));

	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	GLuint uniID = glGetUniformLocation(shaderProgram.ID, "scale");

	glClearColor(0.102f, 0.137f, 0.494f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers(window);

	glEnable(GL_DEPTH_TEST);

	float rotation = 0.0f;
	double prevTime = glfwGetTime();
	
	while (glfwWindowShouldClose(window) == false) {
		glClearColor(0.102f, 0.137f, 0.494f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderProgram.Activate();

		double currTime = glfwGetTime();

		if (currTime - prevTime >= 1 / 60) {
			//rotation += 0.1f;
			prevTime = currTime;
		}

		mat4 model = mat4(1.0f);
		mat4 view = mat4(1.0f);
		mat4 proj = mat4(1.0f);

		model = rotate(model, radians(rotation), vec3(0.0f, 1.0f, 0.0f));
		model = translate(model, vec3(-0.5f, -0.5f, 0.0f));
		model = rotate(model, radians(-45.0f), vec3(0.0f, 0.0f, 1.0f));

		view = translate(view, vec3(0.0f, 0.0f, -2.0f));
		proj = perspective(radians(45.0f), float(width / height), 0.1f, 100.0f);

		GLuint modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

		GLuint viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));

		GLuint projLoc = glGetUniformLocation(shaderProgram.ID, "proj");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(proj));

		glUniform1f(uniID, 0.5);
		VAO1.Bind();
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(GLuint), GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	shaderProgram.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}