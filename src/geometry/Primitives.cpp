#include "Primitives.h"

#include <cmath>

#include <glm/gtc/constants.hpp>

namespace {

constexpr float PI = glm::pi<float>();

// Appends the two triangles (a, b, c) and (c, d, a) of a quad whose corners a-b-c-d are
// listed counter-clockwise as seen from the front.
void addQuad(std::vector<GLuint>& indices, GLuint a, GLuint b, GLuint c, GLuint d)
{
	indices.insert(indices.end(), { a, b, c, c, d, a });
}

} // namespace

namespace Primitives {

// ---------------------------------------------------------------------------------------------
// PLANE — 4 vertices, 2 triangles.
//
//   v3 (-0.5, 0,-0.5) ---- v2 ( 0.5, 0,-0.5)        seen from above (+Y), -Z is "up" on screen
//        |  \                   |
//        |      \      T0 = 0,1,2
//        |          \  T1 = 2,3,0
//   v0 (-0.5, 0, 0.5) ---- v1 ( 0.5, 0, 0.5)
// ---------------------------------------------------------------------------------------------
MeshData Plane()
{
	const glm::vec3 n(0.0f, 1.0f, 0.0f);
	MeshData m;
	m.vertices = {
		{ { -0.5f, 0.0f,  0.5f }, n, { 0.0f, 0.0f } }, // 0 front-left
		{ {  0.5f, 0.0f,  0.5f }, n, { 1.0f, 0.0f } }, // 1 front-right
		{ {  0.5f, 0.0f, -0.5f }, n, { 1.0f, 1.0f } }, // 2 back-right
		{ { -0.5f, 0.0f, -0.5f }, n, { 0.0f, 1.0f } }, // 3 back-left
	};
	addQuad(m.indices, 0, 1, 2, 3);
	return m;
}

// ---------------------------------------------------------------------------------------------
// CUBE — 6 faces x 4 vertices = 24 vertices, 6 faces x 2 triangles = 12 triangles (36 indices).
//
// A cube has only 8 corners, but each corner touches 3 faces that point in 3 different directions.
// A vertex can store only ONE normal and ONE uv, so every corner is duplicated once per face.
// Each face lists its corners counter-clockwise as seen from outside:
// bottom-left, bottom-right, top-right, top-left  (uv = (0,0), (1,0), (1,1), (0,1)).
// ---------------------------------------------------------------------------------------------
MeshData Cube()
{
	struct Face { glm::vec3 normal; glm::vec3 corners[4]; };
	const Face faces[6] = {
		// Front (+Z): viewer at +Z, right = +X, up = +Y
		{ { 0, 0, 1 }, { { -0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f } } },
		// Back (-Z): viewer at -Z, right = -X, up = +Y
		{ { 0, 0, -1 }, { {  0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f } } },
		// Right (+X): viewer at +X, right = -Z, up = +Y
		{ { 1, 0, 0 }, { {  0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f,  0.5f } } },
		// Left (-X): viewer at -X, right = +Z, up = +Y
		{ { -1, 0, 0 }, { { -0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f, -0.5f } } },
		// Top (+Y): viewer above, right = +X, up = -Z
		{ { 0, 1, 0 }, { { -0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f } } },
		// Bottom (-Y): viewer below, right = +X, up = +Z
		{ { 0, -1, 0 }, { { -0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f,  0.5f }, { -0.5f, -0.5f,  0.5f } } },
	};
	const glm::vec2 uvs[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

	MeshData m;
	m.vertices.reserve(24);
	m.indices.reserve(36);
	for (const Face& f : faces) {
		const GLuint base = static_cast<GLuint>(m.vertices.size());
		for (int i = 0; i < 4; ++i)
			m.vertices.push_back({ f.corners[i], f.normal, uvs[i] });
		addQuad(m.indices, base, base + 1, base + 2, base + 3);
	}
	return m;
}

// ---------------------------------------------------------------------------------------------
// SPHERE — latitude/longitude ("UV") sphere.
//
// Ring i (0..stacks) sits at latitude  phi   = PI/2 - i * PI / stacks   (+90 deg top ... -90 deg bottom)
// Column j (0..sectors) sits at longitude theta = j * 2PI / sectors
//
//   x = r cos(phi) sin(theta)     y = r sin(phi)     z = r cos(phi) cos(theta)
//   normal = position / r         uv = (j / sectors, 1 - i / stacks)
//
// Column j = sectors repeats column 0 with u = 1 so the texture seam closes.
// Between ring i (k1) and ring i+1 (k2) each column gives two triangles:
//
//   k1 ---- k1+1         T0 = (k1, k2, k1+1)     skipped on the top ring (degenerate at the pole)
//   |     /  |           T1 = (k1+1, k2, k2+1)   skipped on the bottom ring
//   k2 ---- k2+1
// ---------------------------------------------------------------------------------------------
MeshData Sphere(int stacks, int sectors)
{
	const float r = 0.5f;
	MeshData m;
	m.vertices.reserve(static_cast<size_t>((stacks + 1) * (sectors + 1)));

	for (int i = 0; i <= stacks; ++i) {
		const float phi = PI / 2.0f - static_cast<float>(i) * PI / static_cast<float>(stacks);
		const float ringRadius = r * std::cos(phi);
		const float y = r * std::sin(phi);
		for (int j = 0; j <= sectors; ++j) {
			const float theta = static_cast<float>(j) * 2.0f * PI / static_cast<float>(sectors);
			const glm::vec3 p(ringRadius * std::sin(theta), y, ringRadius * std::cos(theta));
			m.vertices.push_back({ p, p / r,
				{ static_cast<float>(j) / static_cast<float>(sectors), 1.0f - static_cast<float>(i) / static_cast<float>(stacks) } });
		}
	}

	for (int i = 0; i < stacks; ++i) {
		GLuint k1 = static_cast<GLuint>(i * (sectors + 1));
		GLuint k2 = k1 + static_cast<GLuint>(sectors + 1);
		for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
			if (i != 0)
				m.indices.insert(m.indices.end(), { k1, k2, k1 + 1 });
			if (i != stacks - 1)
				m.indices.insert(m.indices.end(), { k1 + 1, k2, k2 + 1 });
		}
	}
	return m;
}

// ---------------------------------------------------------------------------------------------
// CYLINDER — side wall + top cap + bottom cap (separate vertices, because the normals differ).
//
// Side: for every column j, one vertex on the top edge and one on the bottom edge,
//       normal = (sin theta, 0, cos theta) (points straight out), uv = (j / sectors, 1 or 0).
// Caps: a centre vertex plus a ring; triangle fan (centre, j, j+1) on top and (centre, j+1, j)
//       on the bottom, so both caps are counter-clockwise from outside.
// ---------------------------------------------------------------------------------------------
MeshData Cylinder(int sectors)
{
	const float r = 0.5f;
	MeshData m;

	// Side wall
	for (int j = 0; j <= sectors; ++j) {
		const float theta = static_cast<float>(j) * 2.0f * PI / static_cast<float>(sectors);
		const float s = std::sin(theta), c = std::cos(theta);
		const glm::vec3 n(s, 0.0f, c);
		const float u = static_cast<float>(j) / static_cast<float>(sectors);
		m.vertices.push_back({ { r * s,  0.5f, r * c }, n, { u, 1.0f } }); // top    = 2j
		m.vertices.push_back({ { r * s, -0.5f, r * c }, n, { u, 0.0f } }); // bottom = 2j + 1
	}
	for (int j = 0; j < sectors; ++j) {
		const GLuint top = static_cast<GLuint>(2 * j), bottom = top + 1;
		const GLuint nextTop = top + 2, nextBottom = top + 3;
		m.indices.insert(m.indices.end(), { top, bottom, nextTop, nextTop, bottom, nextBottom });
	}

	// Caps
	for (int side = 0; side < 2; ++side) {
		const bool isTop = (side == 0);
		const float y = isTop ? 0.5f : -0.5f;
		const glm::vec3 n(0.0f, isTop ? 1.0f : -1.0f, 0.0f);
		const GLuint center = static_cast<GLuint>(m.vertices.size());
		m.vertices.push_back({ { 0.0f, y, 0.0f }, n, { 0.5f, 0.5f } });
		for (int j = 0; j <= sectors; ++j) {
			const float theta = static_cast<float>(j) * 2.0f * PI / static_cast<float>(sectors);
			const float x = r * std::sin(theta), z = r * std::cos(theta);
			m.vertices.push_back({ { x, y, z }, n, { 0.5f + x, isTop ? 0.5f - z : 0.5f + z } });
		}
		for (int j = 0; j < sectors; ++j) {
			const GLuint a = center + 1 + static_cast<GLuint>(j), b = a + 1;
			if (isTop) m.indices.insert(m.indices.end(), { center, a, b });
			else       m.indices.insert(m.indices.end(), { center, b, a });
		}
	}
	return m;
}

// ---------------------------------------------------------------------------------------------
// CONE — slanted side + base cap.
//
// For a cone of height h = 1 and base radius r = 0.5 the outward side normal at angle theta is
//   n = normalize( h sin(theta),  r,  h cos(theta) )
// The apex is shared by every side triangle but needs a different normal for each, so it is
// duplicated per sector and given the normal of the middle of that sector (smooth shading).
// Side triangle j = (base_j, base_j+1, apex_j), counter-clockwise from outside.
// ---------------------------------------------------------------------------------------------
MeshData Cone(int sectors)
{
	const float r = 0.5f, h = 1.0f;
	MeshData m;

	auto sideNormal = [&](float theta) {
		return glm::normalize(glm::vec3(h * std::sin(theta), r, h * std::cos(theta)));
	};

	for (int j = 0; j <= sectors; ++j) {
		const float theta = static_cast<float>(j) * 2.0f * PI / static_cast<float>(sectors);
		const float u = static_cast<float>(j) / static_cast<float>(sectors);
		m.vertices.push_back({ { r * std::sin(theta), -0.5f, r * std::cos(theta) }, sideNormal(theta), { u, 0.0f } });
	}
	const GLuint apexStart = static_cast<GLuint>(m.vertices.size());
	for (int j = 0; j < sectors; ++j) {
		const float mid = (static_cast<float>(j) + 0.5f) * 2.0f * PI / static_cast<float>(sectors);
		m.vertices.push_back({ { 0.0f, 0.5f, 0.0f }, sideNormal(mid), { (static_cast<float>(j) + 0.5f) / static_cast<float>(sectors), 1.0f } });
	}
	for (int j = 0; j < sectors; ++j) {
		const GLuint a = static_cast<GLuint>(j);
		m.indices.insert(m.indices.end(), { a, a + 1, apexStart + static_cast<GLuint>(j) });
	}

	// Base cap (faces -Y)
	const glm::vec3 n(0.0f, -1.0f, 0.0f);
	const GLuint center = static_cast<GLuint>(m.vertices.size());
	m.vertices.push_back({ { 0.0f, -0.5f, 0.0f }, n, { 0.5f, 0.5f } });
	for (int j = 0; j <= sectors; ++j) {
		const float theta = static_cast<float>(j) * 2.0f * PI / static_cast<float>(sectors);
		const float x = r * std::sin(theta), z = r * std::cos(theta);
		m.vertices.push_back({ { x, -0.5f, z }, n, { 0.5f + x, 0.5f + z } });
	}
	for (int j = 0; j < sectors; ++j) {
		const GLuint a = center + 1 + static_cast<GLuint>(j);
		m.indices.insert(m.indices.end(), { center, a + 1, a });
	}
	return m;
}

}
