#version 330 core
// ============================================================================================
// GPU Whitted ray tracer. One invocation per pixel:
//
//   1. build the primary ray through the pixel from the camera basis
//   2. find the nearest hit by descending a BOUNDING VOLUME HIERARCHY of axis-aligned boxes and
//      testing only the instances in the leaves the ray enters (exact analytic primitives)
//   3. shade it with the SAME Phong model as the rasteriser, plus a SHADOW RAY per light
//   4. continue through transparent surfaces or bounce off reflective ones (uMaxBounces)
//
// Instance layout in uInstances (8 RGBA32F texels each):
//   0..2  rows 0..2 of the inverse model matrix
//   3     color.rgb, primitive type (0 plane, 1 cube, 2 sphere, 3 cylinder, 4 cone)
//   4     ka, kd, ks, shininess
//   5     emissive.rgb, opacity
//   6     reflectivity, texture slot (-1 none), uvScale.x, uvScale.y
//   7     unlit flag, cutout flag (texture alpha < 0.5 = hole)
// BVH nodes follow the instances, from texel uNodeOffset, 2 texels each:
//   0     box min.xyz, first (child index, or first instance of a leaf)
//   1     box max.xyz, count (0 = inner node with children first, first + 1; > 0 = leaf)
// ============================================================================================

#include "lighting.glsl"
#include "fog.glsl"
#include "sky.glsl"

in vec2 vNdc;
out vec4 FragColor;

uniform samplerBuffer uInstances;
uniform sampler2DArray uSurfaceMaps;
uniform int uNodeOffset;
uniform int uLightShadow[MAX_LIGHTS];

uniform vec3 uCamPos;
uniform vec3 uCamForward;
uniform vec3 uCamRight;
uniform vec3 uCamUp;
uniform float uTanHalfFov;
uniform float uAspect;
uniform int uMaxBounces;
uniform int uUseTexture;


const float INF = 1e20;
const float EPS = 1e-4;

vec4 fetch(int instance, int k) { return texelFetch(uInstances, instance * 8 + k); }
vec4 fetchNode(int node, int k) { return texelFetch(uInstances, uNodeOffset + node * 2 + k); }

// ---------------- primitive intersections (object space, mirror of src/math/Ray.cpp) -------

float hitPlane(vec3 o, vec3 d, out vec3 n)
{
	n = vec3(0, 1, 0);
	if (d.y >= 0.0 || o.y <= 0.0) return INF;       // one-sided
	float t = -o.y / d.y;
	vec3 p = o + t * d;
	return (abs(p.x) <= 0.5 && abs(p.z) <= 0.5) ? t : INF;
}

float hitCube(vec3 o, vec3 d, out vec3 n)
{
	vec3 inv = 1.0 / d;
	vec3 t1 = (-0.5 - o) * inv;
	vec3 t2 = (0.5 - o) * inv;
	vec3 tmin = min(t1, t2), tmax = max(t1, t2);
	float tNear = max(max(tmin.x, tmin.y), tmin.z);
	float tFar = min(min(tmax.x, tmax.y), tmax.z);
	n = vec3(0.0);
	if (tNear > tFar || tFar < EPS) return INF;
    if (tNear < EPS) {
        if (tFar == tmax.x) n=vec3(sign(d.x),0,0);
        else if (tFar == tmax.y) n=vec3(0,sign(d.y),0);
        else n=vec3(0,0,sign(d.z));
        return tFar;
    }
	// the axis whose slab was entered last is the face that was hit
	if (tNear == tmin.x) n = vec3(-sign(d.x), 0, 0);
	else if (tNear == tmin.y) n = vec3(0, -sign(d.y), 0);
	else n = vec3(0, 0, -sign(d.z));
	return tNear;
}

float hitSphere(vec3 o, vec3 d, out vec3 n)
{
	float a = dot(d, d), b = 2.0 * dot(o, d), c = dot(o, o) - 0.25;
	float disc = b * b - 4.0 * a * c;
	n = vec3(0.0);
	if (disc < 0.0) return INF;
	float s = sqrt(disc);
	float t = (-b - s) / (2.0 * a);
	if (t < EPS) t = (-b + s) / (2.0 * a);
	if (t < EPS) return INF;
	n = normalize(o + t * d);
	return t;
}

void diskHit(vec3 o, vec3 d, float h, float ny, inout float best, inout vec3 n)
{
	if (abs(d.y) < 1e-8) return;
	float t = (h - o.y) / d.y;
	vec3 p = o + t * d;
	if (t > EPS && t < best && dot(p.xz, p.xz) <= 0.25) { best = t; n = vec3(0, ny, 0); }
}

float hitCylinder(vec3 o, vec3 d, out vec3 n)
{
	float best = INF;
	n = vec3(0.0);
	float a = dot(d.xz, d.xz), b = 2.0 * dot(o.xz, d.xz), c = dot(o.xz, o.xz) - 0.25;
	float disc = b * b - 4.0 * a * c;
	if (a > 1e-8 && disc >= 0.0) {
		float s = sqrt(disc);
		for (int k = 0; k < 2; ++k) {
			float t = (-b + (k == 0 ? -s : s)) / (2.0 * a);
			float y = o.y + t * d.y;
			if (t > EPS && abs(y) <= 0.5 && t < best) {
				best = t;
				vec3 p = o + t * d;
				n = normalize(vec3(p.x, 0.0, p.z));
				break;
			}
		}
	}
	diskHit(o, d, 0.5, 1.0, best, n);
	diskHit(o, d, -0.5, -1.0, best, n);
	return best;
}

float hitCone(vec3 o, vec3 d, out vec3 n)
{
	float best = INF;
	n = vec3(0.0);
	float k2 = 0.25;
	float oy = 0.5 - o.y;
	float a = dot(d.xz, d.xz) - k2 * d.y * d.y;
	float b = 2.0 * (dot(o.xz, d.xz) + k2 * oy * d.y);
	float c = dot(o.xz, o.xz) - k2 * oy * oy;
	float disc = b * b - 4.0 * a * c;
    if (abs(a) <= 1e-8 && abs(b) > 1e-8) {
        float t=-c/b;
        vec3 p=o+t*d;
        if (t>EPS && p.y>=-0.5 && p.y<=0.5) { best=t; n=normalize(vec3(2.0*p.x,2.0*k2*(0.5-p.y),2.0*p.z)); }
    }
	if (abs(a) > 1e-8 && disc >= 0.0) {
		float s = sqrt(disc);
		float r0 = (-b - s) / (2.0 * a), r1 = (-b + s) / (2.0 * a);
		float lo = min(r0, r1), hi = max(r0, r1);
		for (int k = 0; k < 2; ++k) {
			float t = k == 0 ? lo : hi;
			float y = o.y + t * d.y;
			if (t > EPS && y >= -0.5 && y <= 0.5) {
				best = t;
				vec3 p = o + t * d;
				n = normalize(vec3(2.0 * p.x, 2.0 * k2 * (0.5 - p.y), 2.0 * p.z));
				break;
			}
		}
	}
	diskHit(o, d, -0.5, -1.0, best, n);
	return best;
}

// Intersects one instance with a WORLD ray. Returns t (INF = miss), object-space point and normal.
float hitInstance(int i, vec3 ro, vec3 rd, out vec3 objPoint, out vec3 objNormal)
{
	vec4 r0 = fetch(i, 0), r1 = fetch(i, 1), r2 = fetch(i, 2);
	vec3 o = vec3(dot(r0, vec4(ro, 1.0)), dot(r1, vec4(ro, 1.0)), dot(r2, vec4(ro, 1.0)));
	vec3 d = vec3(dot(r0.xyz, rd), dot(r1.xyz, rd), dot(r2.xyz, rd)); // not normalised: t stays world t
	int type = int(fetch(i, 3).w + 0.5);
	float t;
	if (type == 0) t = hitPlane(o, d, objNormal);
	else if (type == 1) t = hitCube(o, d, objNormal);
	else if (type == 2) t = hitSphere(o, d, objNormal);
	else if (type == 3) t = hitCylinder(o, d, objNormal);
	else t = hitCone(o, d, objNormal);
	objPoint = o + t * d;
	return t;
}

vec2 primitiveUV(int type, vec3 p, vec3 n);
float sampleSlotAlpha(int slot, vec2 uv);

// Slab test against an axis-aligned box: entry distance, or INF when missed / beyond tMax.
float hitBox(vec3 ro, vec3 invD, vec3 lo, vec3 hi, float tMax)
{
	vec3 t1 = (lo - ro) * invD, t2 = (hi - ro) * invD;
	vec3 tmin = min(t1, t2), tmax = max(t1, t2);
	float tNear = max(max(tmin.x, tmin.y), max(tmin.z, 0.0));
	float tFar = min(min(tmax.x, tmax.y), tmax.z);
	return (tNear <= tFar && tNear < tMax) ? tNear : INF;
}

vec3 safeInverse(vec3 d)
{
	return 1.0 / vec3(abs(d.x) < 1e-8 ? 1e-8 : d.x, abs(d.y) < 1e-8 ? 1e-8 : d.y, abs(d.z) < 1e-8 ? 1e-8 : d.z);
}

// Cut-out materials (fence, railings): a hit where the texture is transparent does not count.
bool cutAway(int i, vec3 p, vec3 n)
{
	if (fetch(i, 7).y < 0.5) return false;
	vec4 t6 = fetch(i, 6);
	int type = int(fetch(i, 3).w + 0.5);
	return sampleSlotAlpha(int(floor(t6.y + 0.5)), primitiveUV(type, p, n) * t6.zw) < 0.5;
}

// Nearest hit along the ray: depth-first BVH descent, nearer child first, pruned by the best t so far.
int traceClosest(vec3 ro, vec3 rd, out float tHit, out vec3 objPoint, out vec3 objNormal)
{
	int best = -1;
	tHit = INF;
	vec3 invD = safeInverse(rd);
	int stack[32];
	int sp = 0;
	stack[sp++] = 0;
	while (sp > 0) {
		int node = stack[--sp];
		vec4 a = fetchNode(node, 0), b = fetchNode(node, 1);
		if (hitBox(ro, invD, a.xyz, b.xyz, tHit) == INF) continue;
		int first = int(a.w + 0.5), count = int(b.w + 0.5);
		if (count > 0) {
			for (int i = first; i < first + count; ++i) {
				vec3 p, n;
				float t = hitInstance(i, ro, rd, p, n);
				if (t < tHit && !cutAway(i, p, n)) { tHit = t; best = i; objPoint = p; objNormal = n; }
			}
		} else if (sp < 30) {
			float tl = hitBox(ro, invD, fetchNode(first, 0).xyz, fetchNode(first, 1).xyz, tHit);
			float tr = hitBox(ro, invD, fetchNode(first + 1, 0).xyz, fetchNode(first + 1, 1).xyz, tHit);
			// push the farther child first so the nearer one is popped (and tightens tHit) first
			if (tl < tr) { if (tr < INF) stack[sp++] = first + 1; if (tl < INF) stack[sp++] = first; }
			else         { if (tl < INF) stack[sp++] = first;     if (tr < INF) stack[sp++] = first + 1; }
		}
	}
	return best;
}

// Any opaque, lit occluder between the point and the light? (any-hit: stops at the first one)
bool occluded(vec3 ro, vec3 rd, float maxT)
{
	vec3 invD = safeInverse(rd);
	int stack[32];
	int sp = 0;
	stack[sp++] = 0;
	while (sp > 0) {
		int node = stack[--sp];
		vec4 a = fetchNode(node, 0), b = fetchNode(node, 1);
		if (hitBox(ro, invD, a.xyz, b.xyz, maxT) == INF) continue;
		int first = int(a.w + 0.5), count = int(b.w + 0.5);
		if (count > 0) {
			for (int i = first; i < first + count; ++i) {
				if (fetch(i, 7).x > 0.5 || fetch(i, 5).w < 0.5) continue; // light sources / glass cast no shadow
				vec3 p, n;
				if (hitInstance(i, ro, rd, p, n) < maxT && !cutAway(i, p, n)) return true;
			}
		} else if (sp < 30) {
			stack[sp++] = first;
			stack[sp++] = first + 1;
		}
	}
	return false;
}

// Texture coordinates at an object-space hit point; must match the mesh UVs in Primitives.cpp.
vec2 primitiveUV(int type, vec3 p, vec3 n)
{
	const float PI = 3.14159265;
	float u = atan(p.x, p.z) / (2.0 * PI);
	if (u < 0.0) u += 1.0;
	if (type == 0) return vec2(p.x + 0.5, 0.5 - p.z);
	if (type == 2) return vec2(u, 0.5 + asin(clamp(2.0 * p.y, -1.0, 1.0)) / PI);
	if (type == 1) {
		if (n.z > 0.5) return vec2(p.x + 0.5, p.y + 0.5);
		if (n.z < -0.5) return vec2(0.5 - p.x, p.y + 0.5);
		if (n.x > 0.5) return vec2(0.5 - p.z, p.y + 0.5);
		if (n.x < -0.5) return vec2(p.z + 0.5, p.y + 0.5);
		if (n.y > 0.5) return vec2(p.x + 0.5, 0.5 - p.z);
		return vec2(p.x + 0.5, p.z + 0.5);
	}
	// cylinder / cone: caps are planar, the side wraps around
	if (abs(n.y) > 0.99 && (type == 3 || n.y < 0.0))
		return vec2(0.5 + p.x, n.y > 0.0 ? 0.5 - p.z : 0.5 + p.z);
	return vec2(u, p.y + 0.5);
}

vec3 sampleSlot(int slot, vec2 uv)
{
    return texture(uSurfaceMaps, vec3(uv, float(slot))).rgb;
}

// Only the cut-out textures (fence pickets / railings, slot 14) carry a meaningful alpha.
float sampleSlotAlpha(int slot, vec2 uv)
{
	// level 0: neighbouring pixels of a ray tracer can hit unrelated surfaces, so screen-space
	// derivatives (and the mip level chosen from them) are meaningless at a cut-out's edges
	if (slot == 14) return textureLod(uSurfaceMaps, vec3(uv, 14.0), 0.0).a;
	return 1.0;
}

void main()
{
	// 1. Primary ray through this pixel
	vec3 rd = normalize(uCamForward + vNdc.x * uTanHalfFov * uAspect * uCamRight + vNdc.y * uTanHalfFov * uCamUp);
	vec3 ro = uCamPos;

	vec3 color = vec3(0.0);
	vec3 throughput = vec3(1.0);

	float fogDistance=0.0;
	for (int bounce = 0; bounce <= uMaxBounces; ++bounce) {
		float t; vec3 op, on;
		int hit = traceClosest(ro, rd, t, op, on);
		if (bounce==0 && hit>=0) fogDistance=t;
		// A ray that leaves the scene sees the sky dome - also in reflections and through glass.
		if (hit < 0) { color += throughput * skyRadiance(rd); break; }

		// 2. Surface data
		vec4 r0 = fetch(hit, 0), r1 = fetch(hit, 1), r2 = fetch(hit, 2);
		vec4 t3 = fetch(hit, 3), t4 = fetch(hit, 4), t5 = fetch(hit, 5), t6 = fetch(hit, 6), t7 = fetch(hit, 7);
		int type = int(t3.w + 0.5);
		vec3 P = ro + t * rd;
		vec3 N = normalize(on.x * r0.xyz + on.y * r1.xyz + on.z * r2.xyz); // transpose(inverse) * n
		if (dot(N, rd) > 0.0) N = -N;

		vec3 albedo = t3.rgb;
		int slot = int(floor(t6.y + 0.5));
		if (uUseTexture == 1 && slot >= 0)
			albedo *= sampleSlot(slot, primitiveUV(type, op, on) * t6.zw);

		// 3. Local illumination with shadow rays
		vec3 shaded;
		if (t7.x > 0.5) {
			shaded = t5.rgb + albedo;                          // unlit (sky, bulb, sun)
		} else if (uLightingEnabled == 0 || uShadingEnabled == 0) {
			shaded = albedo * (uLightingEnabled == 1 ? basicIllumination(P, t4.x, t4.y) : vec3(1.0)) + t5.rgb;
		} else {
			vec3 V = -rd;
			vec3 diffuse = vec3(0.0), specular = vec3(0.0);
			vec3 origin = P + N * 2e-3;
			for (int i = 0; i < uLightCount; ++i) {
				if (uLights[i].enabled == 0) continue;
				vec3 L; float att; float dist;
				lightVector(i, P, L, att, dist);
				if (att <= 0.0 || dot(N, L) <= 0.0) continue;
				// A shadow ray is a full BVH traversal: only send one where this light visibly matters.
				float strength = att * uLights[i].intensity * dot(N, L);
				if (strength < 0.004) continue;
				float visibility = 1.0;
				if (uLightShadow[i] == 1 && strength > 0.02 && occluded(origin, L, dist - 4e-3)) visibility = 0.0;
				addLight(i, P, N, V, visibility, t4.y, t4.z, t4.w, diffuse, specular);
			}
			shaded = albedo * (ambientTerm(t4.x) + diffuse) + specular + t5.rgb;
		}

		// 4. Local illumination closes the finite path at the last permitted hit.
		if (bounce == uMaxBounces) { color += throughput * shaded; break; }

		// 4. Transparency, reflection or stop
		float opacity = t5.a;
		float reflectivity = t6.x;
		if (opacity < 0.99) {
			color += throughput * shaded * opacity;
			throughput *= (1.0 - opacity);
			ro = P + rd * 2e-3;               // continue straight through
		} else {
			color += throughput * shaded * (1.0 - reflectivity);
			if (reflectivity <= 0.0) break;
			throughput *= reflectivity;
			rd = reflect(rd, N);              // mirror bounce
			ro = P + N * 2e-3;
		}
		if (max(throughput.r, max(throughput.g, throughput.b)) < 0.02) break;
	}

	FragColor = vec4(applyFog(color,fogDistance), 1.0);
}
