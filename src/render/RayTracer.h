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
//   1. CPU: build a BOUNDING VOLUME HIERARCHY (BVH) over the items' world-space boxes, then pack the
//      items in BVH order (inverse model matrix + material) followed by the BVH nodes into one float
//      texture buffer. A ray descends the tree and only tests the few items in the leaves whose boxes it
//      enters: about log2(n) box tests instead of testing every item.
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
	int NodeCount() const { return static_cast<int>(nodes.size()); }

	static constexpr int TexelsPerInstance = 8;
	static constexpr int TexelsPerNode = 2;
	static constexpr int LeafSize = 2;      // at most this many items per BVH leaf

private:
	void EnsureTarget(int w, int h);

	// BVH node: box (lo, hi); leaf if count > 0 (items [first, first + count) in `order`),
	// otherwise its children are nodes `first` and `first + 1`.
	struct Node { glm::vec3 lo, hi; int first = 0, count = 0; };
	struct Box { glm::vec3 lo, hi, centre; };
	void Build(int node, int first, int count);

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
	std::vector<Box> boxes;               // world box of every traced item
	std::vector<int> order;               // item indices in BVH leaf order
	std::vector<int> boxLookup;           // item index -> its entry in `boxes`
	std::vector<Node> nodes;
};
