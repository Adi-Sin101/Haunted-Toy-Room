#include "Transform3D.h"

#include <cmath>
#include <algorithm>

namespace t3d {

glm::mat4 fromRows(float m00, float m01, float m02, float m03,
                   float m10, float m11, float m12, float m13,
                   float m20, float m21, float m22, float m23,
                   float m30, float m31, float m32, float m33)
{
	// glm::mat4 constructor takes COLUMNS, so feed it the transposed data.
	return glm::mat4(m00, m10, m20, m30,
	                 m01, m11, m21, m31,
	                 m02, m12, m22, m32,
	                 m03, m13, m23, m33);
}

//          | 1 0 0 tx |
//  T(t) =  | 0 1 0 ty |
//          | 0 0 1 tz |
//          | 0 0 0 1  |
glm::mat4 translate(const glm::vec3& t)
{
	return fromRows(1, 0, 0, t.x,
	                0, 1, 0, t.y,
	                0, 0, 1, t.z,
	                0, 0, 0, 1);
}

//          | sx 0  0  0 |
//  S(s) =  | 0  sy 0  0 |
//          | 0  0  sz 0 |
//          | 0  0  0  1 |
glm::mat4 scale(const glm::vec3& s)
{
	return fromRows(s.x, 0, 0, 0,
	                0, s.y, 0, 0,
	                0, 0, s.z, 0,
	                0, 0, 0, 1);
}

// Rotation about X: y' = y cos - z sin,  z' = y sin + z cos
glm::mat4 rotateX(float a)
{
	const float c = std::cos(a), s = std::sin(a);
	return fromRows(1, 0, 0, 0,
	                0, c, -s, 0,
	                0, s, c, 0,
	                0, 0, 0, 1);
}

// Rotation about Y: z' = z cos - x sin,  x' = z sin + x cos
glm::mat4 rotateY(float a)
{
	const float c = std::cos(a), s = std::sin(a);
	return fromRows(c, 0, s, 0,
	                0, 1, 0, 0,
	                -s, 0, c, 0,
	                0, 0, 0, 1);
}

// Rotation about Z: x' = x cos - y sin,  y' = x sin + y cos
glm::mat4 rotateZ(float a)
{
	const float c = std::cos(a), s = std::sin(a);
	return fromRows(c, -s, 0, 0,
	                s, c, 0, 0,
	                0, 0, 1, 0,
	                0, 0, 0, 1);
}

// Rotation by angle a about the unit axis u = (x, y, z) through the origin (Rodrigues):
//   R = cos(a) I + (1 - cos(a)) u u^T + sin(a) [u]x
glm::mat4 rotateAxis(const glm::vec3& axis, float a)
{
	const glm::vec3 u = glm::normalize(axis);
	const float c = std::cos(a), s = std::sin(a), t = 1.0f - c;
	const float x = u.x, y = u.y, z = u.z;
	return fromRows(t * x * x + c,     t * x * y - s * z, t * x * z + s * y, 0,
	                t * x * y + s * z, t * y * y + c,     t * y * z - s * x, 0,
	                t * x * z - s * y, t * y * z + s * x, t * z * z + c,     0,
	                0, 0, 0, 1);
}

glm::mat4 shear(float xy, float xz, float yx, float yz, float zx, float zy)
{
	return fromRows(1, xy, xz, 0,
	                yx, 1, yz, 0,
	                zx, zy, 1, 0,
	                0, 0, 0, 1);
}

glm::mat4 reflect(const glm::vec3& planeNormal)
{
	const glm::vec3 n = glm::normalize(planeNormal);
	return fromRows(1 - 2 * n.x * n.x, -2 * n.x * n.y,    -2 * n.x * n.z,    0,
	                -2 * n.y * n.x,    1 - 2 * n.y * n.y, -2 * n.y * n.z,    0,
	                -2 * n.z * n.x,    -2 * n.z * n.y,    1 - 2 * n.z * n.z, 0,
	                0, 0, 0, 1);
}

// Scaling about a fixed point p: move p to the origin, scale, move back.
glm::mat4 scaleAboutPoint(const glm::vec3& s, const glm::vec3& p)
{
	return translate(p) * scale(s) * translate(-p);
}

// Rotation about an arbitrary axis passing through point p.
glm::mat4 rotateAboutAxis(const glm::vec3& p, const glm::vec3& axis, float a)
{
	return translate(p) * rotateAxis(axis, a) * translate(-p);
}

// View matrix: builds the camera's orthonormal basis (right r, up u, forward f) and expresses
// world points in it. Rows are r, u, -f (the camera looks down its own -Z axis).
glm::mat4 lookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up)
{
	const glm::vec3 f = glm::normalize(target - eye);
	const glm::vec3 r = glm::normalize(glm::cross(f, up));
	const glm::vec3 u = glm::cross(r, f);
	return fromRows(r.x, r.y, r.z, -glm::dot(r, eye),
	                u.x, u.y, u.z, -glm::dot(u, eye),
	                -f.x, -f.y, -f.z, glm::dot(f, eye),
	                0, 0, 0, 1);
}

// Perspective projection: maps the view frustum to the clip cube. w' = -z gives the divide by depth.
glm::mat4 perspective(float fovy, float aspect, float n, float f)
{
	const float t = std::tan(fovy / 2.0f);
	return fromRows(1.0f / (aspect * t), 0, 0, 0,
	                0, 1.0f / t, 0, 0,
	                0, 0, -(f + n) / (f - n), -2.0f * f * n / (f - n),
	                0, 0, -1, 0);
}

// Orthographic projection: a box is scaled into the clip cube with no divide (w' = 1).
glm::mat4 orthographic(float halfWidth, float halfHeight, float n, float f)
{
	return fromRows(1.0f / halfWidth, 0, 0, 0,
	                0, 1.0f / halfHeight, 0, 0,
	                0, 0, -2.0f / (f - n), -(f + n) / (f - n),
	                0, 0, 0, 1);
}

glm::mat3 normalMatrix(const glm::mat4& model)
{
	return glm::transpose(glm::inverse(glm::mat3(model)));
}

float unitBoundsRadius(const glm::mat4& model)
{
	const glm::vec3 a(model[0]), b(model[1]), c(model[2]);
	// Opposite corners have equal length, so only four sign combinations are needed.
	float squared = 0.0f;
	for (float y : {-1.0f, 1.0f}) for (float z : {-1.0f, 1.0f}) {
		const glm::vec3 corner = a + y * b + z * c;
		squared = std::max(squared, glm::dot(corner, corner));
	}
	return 0.5f * std::sqrt(squared);
}

}
