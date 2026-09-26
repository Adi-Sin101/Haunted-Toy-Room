#pragma once

#include <glm/glm.hpp>

#include "geometry/Mesh.h"

// A ray p(t) = origin + t * direction.
struct Ray {
	glm::vec3 origin;
	glm::vec3 direction;
};

struct RayHit {
	float t = -1.0f;          // ray parameter of the nearest hit (< 0 = miss)
	glm::vec3 normal{ 0.0f }; // object-space surface normal at the hit
};

// Exact ray intersection with the unit primitives of Primitives.h, in OBJECT space.
//
// To intersect an object with model matrix M, transform the WORLD ray into object space with M^-1:
//   o' = M^-1 * (o, 1)      d' = M^-1 * (d, 0)
// d' is NOT renormalised, so the t found in object space is the same t as in world space.
// This is how the GPU ray tracer handles every sphere/cube/cylinder/cone/plane with one routine each.
namespace RayIntersect {

RayHit Plane(const Ray& r);    // y = 0, |x|,|z| <= 0.5, one-sided (front face +Y only)
RayHit Cube(const Ray& r);     // slab method on [-0.5, 0.5]^3
RayHit Sphere(const Ray& r);   // |p|^2 = 0.25
RayHit Cylinder(const Ray& r); // x^2 + z^2 = 0.25, |y| <= 0.5, plus caps
RayHit Cone(const Ray& r);     // x^2 + z^2 = (0.5 (0.5 - y))^2, y in [-0.5, 0.5], plus base cap

RayHit Primitive(PrimitiveType type, const Ray& r);

// Transforms a world ray by the inverse model matrix and intersects the primitive.
RayHit Object(PrimitiveType type, const glm::mat4& inverseModel, const Ray& worldRay);

}
