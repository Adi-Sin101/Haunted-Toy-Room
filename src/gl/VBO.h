#ifndef VBO_CLASS_H
#define VBO_CLASS_H

#include <glad/glad.h>
#include <span>

#include "geometry/Vertex.h"

// Vertex Buffer Object: a block of GPU memory holding vertex data.
// Owns its GL handle (move-only), so the buffer is freed automatically.
class VBO {
public:
	GLuint ID = 0;

	VBO() = default;
	VBO(const GLfloat* vertices, GLsizeiptr size);
	explicit VBO(std::span<const Vertex> vertices);
	~VBO();

	VBO(const VBO&) = delete;
	VBO& operator=(const VBO&) = delete;
	VBO(VBO&& other) noexcept;
	VBO& operator=(VBO&& other) noexcept;

	// Allocates (or re-allocates) the buffer and copies data into it.
	void Upload(const void* data, GLsizeiptr size, GLenum usage = GL_STATIC_DRAW);

	void Bind() const;
	void Unbind() const;
	void Delete();
};

#endif
