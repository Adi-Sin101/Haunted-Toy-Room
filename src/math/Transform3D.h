#pragma once

#include <glm/glm.hpp>

// Hand-written 3D transformation matrices (Lecture 3: 3D Transformation).
//
// Every function builds its 4x4 homogeneous matrix entry by entry, written in the same ROW-MAJOR
// layout as the lecture slides. GLM stores matrices column-major (m[column][row]), so fromRows()
// performs the transpose once; everything else is plain maths.
//
// Points are column vectors: p' = M * p, so in a product A * B * p, B is applied first.
namespace t3d {

glm::mat4 fromRows(float m00, float m01, float m02, float m03,
                   float m10, float m11, float m12, float m13,
                   float m20, float m21, float m22, float m23,
                   float m30, float m31, float m32, float m33);

// Basic transformations
glm::mat4 translate(const glm::vec3& t);
glm::mat4 scale(const glm::vec3& s);
glm::mat4 rotateX(float radians);
glm::mat4 rotateY(float radians);
glm::mat4 rotateZ(float radians);
glm::mat4 rotateAxis(const glm::vec3& axis, float radians); // Rodrigues' formula, axis through origin

// Shear: x' = x + xy*y + xz*z,  y' = y + yx*x + yz*z,  z' = z + zx*x + zy*y
glm::mat4 shear(float xy, float xz, float yx, float yz, float zx, float zy);

// Reflection about the plane through the origin with unit normal n:  M = I - 2 n n^T
glm::mat4 reflect(const glm::vec3& planeNormal);

// Composite transformations about a fixed point / arbitrary axis (T(p) * M * T(-p))
glm::mat4 scaleAboutPoint(const glm::vec3& s, const glm::vec3& fixedPoint);
glm::mat4 rotateAboutAxis(const glm::vec3& pointOnAxis, const glm::vec3& axis, float radians);

// Viewing and projection
glm::mat4 lookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up);
glm::mat4 perspective(float fovyRadians, float aspect, float zNear, float zFar);

// Normal matrix: transforms normals correctly even under non-uniform scale or shear.
glm::mat3 normalMatrix(const glm::mat4& model);

}
