#include "VBO.h"

#include <utility>

VBO::VBO(const GLfloat* vertices, GLsizeiptr size) {
	Upload(vertices, size);
}

VBO::VBO(std::span<const Vertex> vertices) {
	Upload(vertices.data(), static_cast<GLsizeiptr>(vertices.size_bytes()));
}

VBO::~VBO() {
	Delete();
}

VBO::VBO(VBO&& other) noexcept : ID(std::exchange(other.ID, 0)) {}

VBO& VBO::operator=(VBO&& other) noexcept {
	if (this != &other) {
		Delete();
		ID = std::exchange(other.ID, 0);
	}
	return *this;
}

void VBO::Upload(const void* data, GLsizeiptr size, GLenum usage) {
	if (ID == 0)
		glGenBuffers(1, &ID);
	glBindBuffer(GL_ARRAY_BUFFER, ID);
	glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}

void VBO::Bind() const {
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}

void VBO::Unbind() const {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::Delete() {
	if (ID != 0) {
		glDeleteBuffers(1, &ID);
		ID = 0;
	}
}
