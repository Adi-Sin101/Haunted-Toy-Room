#ifndef EBO_CLASS_H
#define EBO_CLASS_H

#include <glad/glad.h>
#include <span>

// Element Buffer Object: holds the indices that say which vertices form each triangle.
// Must be created while the owning VAO is bound, because the VAO records the EBO binding.
class EBO {
public:
	GLuint ID = 0;

	EBO() = default;
	EBO(const GLuint* indices, GLsizeiptr size);
	explicit EBO(std::span<const GLuint> indices);
	~EBO();

	EBO(const EBO&) = delete;
	EBO& operator=(const EBO&) = delete;
	EBO(EBO&& other) noexcept;
	EBO& operator=(EBO&& other) noexcept;

	void Upload(const GLuint* indices, GLsizeiptr size);

	void Bind() const;
	void Unbind() const;
	void Delete();
};

#endif
