#ifndef VAO_CLASS_H
#define VAO_CLASS_H

#include <glad/glad.h>

#include "VBO.h"

// Vertex Array Object: remembers the vertex layout (which attribute reads which bytes of which VBO)
// and the bound EBO, so drawing a mesh only needs one Bind() call.
class VAO {
public:
	GLuint ID = 0;

	VAO();
	~VAO();

	VAO(const VAO&) = delete;
	VAO& operator=(const VAO&) = delete;
	VAO(VAO&& other) noexcept;
	VAO& operator=(VAO&& other) noexcept;

	// layout        : attribute location in the vertex shader (layout(location = N))
	// numComponents : how many values the attribute has (3 for vec3, 2 for vec2)
	// stride        : bytes from one vertex to the next
	// offset        : byte offset of this attribute inside one vertex
	void LinkAttrib(const VBO& vbo, GLuint layout, GLint numComponents, GLenum type, GLsizei stride, const void* offset);
	void Bind() const;
	void Unbind() const;
	void Delete();
};

#endif
