#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>

#include "Vertex.h"
#include "gl/EBO.h"
#include "gl/VAO.h"
#include "gl/VBO.h"

// The analytic shape a mesh represents. The ray tracer intersects these shapes exactly
// (in the mesh's unit object space) instead of testing every triangle.
enum class PrimitiveType : int {
	Plane = 0,
	Cube = 1,
	Sphere = 2,
	Cylinder = 3,
	Cone = 4,
};

// CPU-side geometry: the vertex list and the index list (3 indices = 1 triangle).
struct MeshData {
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
};

// GPU-side geometry: uploads MeshData once into a VAO + VBO + EBO and draws it with glDrawElements.
// The CPU copy is kept for the vertex/triangle inspector and the normal visualiser.
class Mesh {
public:
	Mesh(std::string name, PrimitiveType type, MeshData data);

	void Draw() const;
	void DrawPoints() const;

	// Level of detail: cheaper versions of the same shape (same PrimitiveType, fewer triangles).
	// `screenSize` = bounding radius / distance to the camera; small or distant objects pick a
	// coarser mesh, because their extra triangles would be smaller than a pixel anyway.
	void SetDetailLevels(const Mesh* medium, const Mesh* low) { mediumDetail = medium; lowDetail = low; }
	const Mesh& ForScreenSize(float screenSize) const
	{
		if (lowDetail && screenSize < 0.012f) return *lowDetail;
		if (mediumDetail && screenSize < 0.06f) return *mediumDetail;
		return *this;
	}

	void Bind() const { vao.Bind(); }
	void DrawBound() const { glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr); }

	const std::string& Name() const { return name; }
	PrimitiveType Type() const { return type; }
	const MeshData& Data() const { return data; }
	GLsizei IndexCount() const { return indexCount; }
	size_t TriangleCount() const { return data.indices.size() / 3; }

private:
	std::string name;
	PrimitiveType type;
	MeshData data;
	GLsizei indexCount = 0;
	const Mesh* mediumDetail = nullptr;
	const Mesh* lowDetail = nullptr;

	VAO vao;
	VBO vbo;
	EBO ebo;
};
