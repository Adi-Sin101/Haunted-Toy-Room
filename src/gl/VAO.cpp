#include "VAO.h"

#include <utility>

VAO::VAO() {
	glGenVertexArrays(1, &ID);
}

VAO::~VAO() {
	Delete();
}

VAO::VAO(VAO&& other) noexcept : ID(std::exchange(other.ID, 0)) {}

VAO& VAO::operator=(VAO&& other) noexcept {
	if (this != &other) {
		Delete();
		ID = std::exchange(other.ID, 0);
	}
	return *this;
}

void VAO::LinkAttrib(const VBO& vbo, GLuint layout, GLint numComponents, GLenum type, GLsizei stride, const void* offset) {
	vbo.Bind();
	glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
	glEnableVertexAttribArray(layout);
	vbo.Unbind();
}

void VAO::Bind() const {
	glBindVertexArray(ID);
}

void VAO::Unbind() const {
	glBindVertexArray(0);
}

void VAO::Delete() {
	if (ID != 0) {
		glDeleteVertexArrays(1, &ID);
		ID = 0;
	}
}
