#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "RenderSettings.h"
#include "gl/Shader.h"

class Assets;
class Renderer;
struct DrawItem;
struct FrameInfo;

// GPU Whitted-style ray tracer (toggle with F4).
//
// Instead of triangles, every draw item is ray traced as its exact analytic primitive:
//   1. CPU: for each item, pack its INVERSE model matrix + material into a float texture buffer.
//      Items are grouped per object (Woody, Bullseye, ...) with a bounding sphere per group, so a
//      ray only tests the parts of objects whose sphere it actually hits.
//   2. GPU: a full-screen triangle runs raytrace.frag once per pixel: primary ray -> nearest hit
//      -> Phong lighting with SHADOW RAYS -> REFLECTION / TRANSPARENCY bounces.
//   3. The image is rendered at a reduced resolution into a framebuffer and drawn scaled up.
class RayTracer {
public:
	void Init(const Assets& assets);
	~RayTracer();

	void Render(const std::vector<DrawItem>& items, const FrameInfo& frame, const RenderSettings& settings,
		const Renderer& renderer, int width, int height);

	int InstanceCount() const { return instanceCount; }

	static constexpr int MaxGroups = 32;
	static constexpr int TexelsPerInstance = 8;

private:
	void EnsureTarget(int w, int h);

	const Assets* assets = nullptr;
	Shader shader;
	Shader presentShader;
	GLuint instanceBuffer = 0;  // GL_TEXTURE_BUFFER storage
	GLuint instanceTexture = 0; // samplerBuffer view of it
	GLuint fbo = 0;
	GLuint colorTexture = 0;
	GLuint emptyVao = 0;
	int targetW = 0, targetH = 0;
	int instanceCount = 0;

	std::vector<glm::vec4> packed;        // reused every frame (no per-frame allocation)
	std::vector<std::vector<int>> buckets; // item indices per group
};
