#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "gl/VAO.h"
#include "gl/VBO.h"

// Batches coloured line segments (normals, axis gizmo, laser guide) into one dynamic VBO and draws
// them with a single glDrawArrays(GL_LINES) call per frame.
class DebugLines {
public:
	DebugLines();

	void Clear() { data.clear(); }
	void Add(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color);
	void Draw(); // uploads and draws; the debug shader must be active
	bool Empty() const { return data.empty(); }

private:
	struct LineVertex { glm::vec3 position; glm::vec3 color; };
	std::vector<LineVertex> data;
	VAO vao;
	VBO vbo;
};
