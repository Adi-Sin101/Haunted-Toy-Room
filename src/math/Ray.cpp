#include "Ray.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr float Eps = 1e-4f;

// Smallest positive root of a t^2 + b t + c = 0 that also passes `accept`, or -1.
template <typename Accept>
float SolveQuadratic(float a, float b, float c, Accept&& accept)
{
	if (std::abs(a) < 1e-8f) {
		if (std::abs(b) < 1e-8f) return -1.0f;
		const float t = -c / b;
		return t > Eps && accept(t) ? t : -1.0f;
	}
	const float disc = b * b - 4.0f * a * c;
	if (disc < 0.0f)
		return -1.0f;
	const float s = std::sqrt(disc);
	const float t0 = (-b - s) / (2.0f * a);
	const float t1 = (-b + s) / (2.0f * a);
	const float lo = std::min(t0, t1), hi = std::max(t0, t1);
	if (lo > Eps && accept(lo)) return lo;
	if (hi > Eps && accept(hi)) return hi;
	return -1.0f;
}

// Keeps the nearer of two hits.
void Closer(RayHit& best, float t, const glm::vec3& n)
{
	if (t > Eps && (best.t < 0.0f || t < best.t)) {
		best.t = t;
		best.normal = n;
	}
}

// Disk of radius 0.5 in the plane y = h, facing +Y (up = true) or -Y.
void Disk(const Ray& r, float h, bool up, RayHit& best)
{
	if (std::abs(r.direction.y) < 1e-8f)
		return;
	const float t = (h - r.origin.y) / r.direction.y;
	const glm::vec3 p = r.origin + t * r.direction;
	if (p.x * p.x + p.z * p.z <= 0.25f)
		Closer(best, t, { 0.0f, up ? 1.0f : -1.0f, 0.0f });
}

} // namespace

namespace RayIntersect {

RayHit Plane(const Ray& r)
{
	RayHit hit;
	// One-sided: only hit from above (the walls are seen from inside the room only).
	if (r.direction.y >= 0.0f || r.origin.y <= 0.0f)
		return hit;
	const float t = -r.origin.y / r.direction.y;
	const glm::vec3 p = r.origin + t * r.direction;
	if (std::abs(p.x) <= 0.5f && std::abs(p.z) <= 0.5f && t > Eps) {
		hit.t = t;
		hit.normal = { 0.0f, 1.0f, 0.0f };
	}
	return hit;
}

// Slab method: the ray is inside the box where it is inside all three pairs of parallel planes.
RayHit Cube(const Ray& r)
{
	RayHit hit;
	float tNear = -std::numeric_limits<float>::max();
	float tFar = std::numeric_limits<float>::max();
	int nearAxis = 0;
	float nearSign = 1.0f;
	int farAxis = 0;
	float farSign = 1.0f;
	for (int axis = 0; axis < 3; ++axis) {
		const float o = r.origin[axis], d = r.direction[axis];
		if (std::abs(d) < 1e-8f) {
			if (o < -0.5f || o > 0.5f)
				return hit;
			continue;
		}
		float t1 = (-0.5f - o) / d, t2 = (0.5f - o) / d;
		float sign = -1.0f; // entering through the -side face
		if (t1 > t2) { std::swap(t1, t2); sign = 1.0f; }
		if (t1 > tNear) { tNear = t1; nearAxis = axis; nearSign = sign; }
		if (t2 < tFar) { tFar=t2; farAxis=axis; farSign=-sign; }
		if (tNear > tFar || tFar < Eps)
			return hit;
	}
	if (tNear > Eps) {
		hit.t = tNear;
		hit.normal = glm::vec3(0.0f);
		hit.normal[nearAxis] = nearSign;
	}
	else if (tFar > Eps) {
		hit.t=tFar;
		hit.normal=glm::vec3(0.0f);
		hit.normal[farAxis]=farSign;
	}
	return hit;
}

// |o + t d|^2 = r^2  ->  (d.d) t^2 + 2 (o.d) t + (o.o - r^2) = 0
RayHit Sphere(const Ray& r)
{
	RayHit hit;
	const float a = glm::dot(r.direction, r.direction);
	const float b = 2.0f * glm::dot(r.origin, r.direction);
	const float c = glm::dot(r.origin, r.origin) - 0.25f;
	const float t = SolveQuadratic(a, b, c, [](float) { return true; });
	if (t > 0.0f) {
		hit.t = t;
		hit.normal = glm::normalize(r.origin + t * r.direction);
	}
	return hit;
}

// Side: (ox + t dx)^2 + (oz + t dz)^2 = 0.25 with |y| <= 0.5; caps: disks at y = +-0.5.
RayHit Cylinder(const Ray& r)
{
	RayHit best;
	const float a = r.direction.x * r.direction.x + r.direction.z * r.direction.z;
	const float b = 2.0f * (r.origin.x * r.direction.x + r.origin.z * r.direction.z);
	const float c = r.origin.x * r.origin.x + r.origin.z * r.origin.z - 0.25f;
	const float t = SolveQuadratic(a, b, c, [&](float tt) {
		return std::abs(r.origin.y + tt * r.direction.y) <= 0.5f;
	});
	if (t > 0.0f) {
		const glm::vec3 p = r.origin + t * r.direction;
		Closer(best, t, glm::normalize(glm::vec3(p.x, 0.0f, p.z)));
	}
	Disk(r, 0.5f, true, best);
	Disk(r, -0.5f, false, best);
	return best;
}

// Side: x^2 + z^2 - k^2 (0.5 - y)^2 = 0 with k = 0.5, y in [-0.5, 0.5]. Normal = gradient.
RayHit Cone(const Ray& r)
{
	RayHit best;
	const float k2 = 0.25f;
	const glm::vec3 o = r.origin, d = r.direction;
	const float oy = 0.5f - o.y; // distance below the apex
	const float a = d.x * d.x + d.z * d.z - k2 * d.y * d.y;
	const float b = 2.0f * (o.x * d.x + o.z * d.z + k2 * oy * d.y);
	const float c = o.x * o.x + o.z * o.z - k2 * oy * oy;
	const float t = SolveQuadratic(a, b, c, [&](float tt) {
		const float y = o.y + tt * d.y;
		return y >= -0.5f && y <= 0.5f;
	});
	if (t > 0.0f) {
		const glm::vec3 p = o + t * d;
		Closer(best, t, glm::normalize(glm::vec3(2.0f * p.x, 2.0f * k2 * (0.5f - p.y), 2.0f * p.z)));
	}
	Disk(r, -0.5f, false, best);
	return best;
}

RayHit Primitive(PrimitiveType type, const Ray& r)
{
	switch (type) {
	case PrimitiveType::Plane: return Plane(r);
	case PrimitiveType::Cube: return Cube(r);
	case PrimitiveType::Sphere: return Sphere(r);
	case PrimitiveType::Cylinder: return Cylinder(r);
	case PrimitiveType::Cone: return Cone(r);
	}
	return {};
}

RayHit Object(PrimitiveType type, const glm::mat4& inverseModel, const Ray& worldRay)
{
	Ray local;
	local.origin = glm::vec3(inverseModel * glm::vec4(worldRay.origin, 1.0f));
	local.direction = glm::vec3(inverseModel * glm::vec4(worldRay.direction, 0.0f));
	return Primitive(type, local);
}

}
