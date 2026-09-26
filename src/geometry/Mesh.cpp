#include "Mesh.h"

#include <cstddef>
#include <utility>

Mesh::Mesh(std::string name, PrimitiveType type, MeshData data)
	: name(std::move(name)), type(type), data(std::move(data))
{
	indexCount = static_cast<GLsizei>(this->data.indices.size());

	// 1. Bind the VAO first: every buffer/attribute setting below is recorded into it.
	vao.Bind();

	// 2. Copy vertices into the VBO and indices into the EBO.
	vbo.Upload(this->data.vertices.data(), static_cast<GLsizeiptr>(this->data.vertices.size() * sizeof(Vertex)));
	ebo.Upload(this->data.indices.data(), static_cast<GLsizeiptr>(this->data.indices.size() * sizeof(GLuint)));

	// 3. Describe the interleaved layout: position, normal, uv.
	constexpr GLsizei stride = sizeof(Vertex);
	vao.LinkAttrib(vbo, 0, 3, GL_FLOAT, stride, reinterpret_cast<const void*>(offsetof(Vertex, position)));
	vao.LinkAttrib(vbo, 1, 3, GL_FLOAT, stride, reinterpret_cast<const void*>(offsetof(Vertex, normal)));
	vao.LinkAttrib(vbo, 2, 2, GL_FLOAT, stride, reinterpret_cast<const void*>(offsetof(Vertex, uv)));

	// 4. Unbind the VAO before the EBO, otherwise the VAO would forget its index buffer.
	vao.Unbind();
	vbo.Unbind();
}

void Mesh::Draw() const
{
	vao.Bind();
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::DrawPoints() const
{
	vao.Bind();
	glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(data.vertices.size()));
}
