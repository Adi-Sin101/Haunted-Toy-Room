#include "DebugLines.h"

#include <cstddef>

DebugLines::DebugLines()
{
	data.reserve(4096);
	vao.Bind();
	vbo.Upload(nullptr, 0, GL_DYNAMIC_DRAW);
	vao.LinkAttrib(vbo, 0, 3, GL_FLOAT, sizeof(LineVertex), reinterpret_cast<const void*>(offsetof(LineVertex, position)));
	vao.LinkAttrib(vbo, 1, 3, GL_FLOAT, sizeof(LineVertex), reinterpret_cast<const void*>(offsetof(LineVertex, color)));
	vao.Unbind();
}

void DebugLines::Add(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color)
{
	data.push_back({ a, color });
	data.push_back({ b, color });
}

void DebugLines::Draw()
{
	if (data.empty())
		return;
	// Orphan + refill: lets the driver hand us fresh memory instead of waiting for the GPU.
	vbo.Upload(data.data(), static_cast<GLsizeiptr>(data.size() * sizeof(LineVertex)), GL_DYNAMIC_DRAW);
	vao.Bind();
	glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(data.size()));
	vao.Unbind();
}
