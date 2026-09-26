#include "EBO.h"

#include <utility>

EBO::EBO(const GLuint* indices, GLsizeiptr size) {
	Upload(indices, size);
}

EBO::EBO(std::span<const GLuint> indices) {
	Upload(indices.data(), static_cast<GLsizeiptr>(indices.size_bytes()));
}

EBO::~EBO() {
	Delete();
}

EBO::EBO(EBO&& other) noexcept : ID(std::exchange(other.ID, 0)) {}

EBO& EBO::operator=(EBO&& other) noexcept {
	if (this != &other) {
		Delete();
		ID = std::exchange(other.ID, 0);
	}
	return *this;
}

void EBO::Upload(const GLuint* indices, GLsizeiptr size) {
	if (ID == 0)
		glGenBuffers(1, &ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
}

void EBO::Bind() const {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}

void EBO::Unbind() const {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void EBO::Delete() {
	if (ID != 0) {
		glDeleteBuffers(1, &ID);
		ID = 0;
	}
}
