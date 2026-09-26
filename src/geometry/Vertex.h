#pragma once

#include <glm/glm.hpp>

// One vertex as stored in the VBO (interleaved, 32 bytes):
//
//   byte  0..11  position  (x, y, z)   -> layout(location = 0)
//   byte 12..23  normal    (nx,ny,nz)  -> layout(location = 1)
//   byte 24..31  uv        (u, v)      -> layout(location = 2)
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

static_assert(sizeof(Vertex) == 8 * sizeof(float), "Vertex must be tightly packed");
