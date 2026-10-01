#include "RayTracer.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "Assets.h"
#include "Renderer.h"
#include "geometry/Mesh.h"
#include "scene/Camera.h"
#include "scene/Light.h"
#include "scene/Material.h"

void RayTracer::Init(const Assets& library)
{
	assets = &library;
	shader = Shader("shaders/raytrace.vert", "shaders/raytrace.frag");
	presentShader = Shader("shaders/raytrace.vert", "shaders/present.frag");

	glGenBuffers(1, &instanceBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, instanceBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec4) * TexelsPerInstance, nullptr, GL_DYNAMIC_DRAW);
	glGenTextures(1, &instanceTexture);
	glBindTexture(GL_TEXTURE_BUFFER, instanceTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, instanceBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	glBindTexture(GL_TEXTURE_BUFFER, 0);

	glGenVertexArrays(1, &emptyVao); // the full-screen triangle is generated from gl_VertexID
	packed.reserve(1024 * TexelsPerInstance);
}

RayTracer::~RayTracer()
{
	if (instanceTexture) glDeleteTextures(1, &instanceTexture);
	if (instanceBuffer) glDeleteBuffers(1, &instanceBuffer);
	if (colorTexture) glDeleteTextures(1, &colorTexture);
	if (fbo) glDeleteFramebuffers(1, &fbo);
	if (emptyVao) glDeleteVertexArrays(1, &emptyVao);
}

void RayTracer::Build(int node, int first, int count)
{
	auto box = [&](int k) -> const Box& { return boxes[static_cast<size_t>(boxLookup[static_cast<size_t>(order[static_cast<size_t>(k)])])]; };
	Node& n = nodes[static_cast<size_t>(node)];
	n.lo = glm::vec3(1e30f);
	n.hi = glm::vec3(-1e30f);
	glm::vec3 clo(1e30f), chi(-1e30f);
	for (int k = first; k < first + count; ++k) {
		n.lo = glm::min(n.lo, box(k).lo);
		n.hi = glm::max(n.hi, box(k).hi);
		clo = glm::min(clo, box(k).centre);
		chi = glm::max(chi, box(k).centre);
	}
	const glm::vec3 extent = chi - clo;
	const int axis = extent.x > extent.y ? (extent.x > extent.z ? 0 : 2) : (extent.y > extent.z ? 1 : 2);
	if (count <= LeafSize || extent[axis] < 1e-5f) {
		n.first = first;
		n.count = count;
		return;
	}
	// Median split: the first half of the items (by centre along the axis) goes left.
	const int mid = first + count / 2;
	std::nth_element(order.begin() + first, order.begin() + mid, order.begin() + first + count, [&](int a, int b) {
		return boxes[static_cast<size_t>(boxLookup[static_cast<size_t>(a)])].centre[axis]
			< boxes[static_cast<size_t>(boxLookup[static_cast<size_t>(b)])].centre[axis];
	});
	const int left = static_cast<int>(nodes.size());
	nodes.push_back({});
	nodes.push_back({});
	nodes[static_cast<size_t>(node)].first = left;   // `n` may dangle after push_back: index again
	nodes[static_cast<size_t>(node)].count = 0;
	Build(left, first, mid - first);
	Build(left + 1, mid, first + count - mid);
}

void RayTracer::EnsureTarget(int w, int h)
{
	if (fbo && w == targetW && h == targetH)
		return;
	targetW = w;
	targetH = h;
	if (!fbo)
		glGenFramebuffers(1, &fbo);
	if (!colorTexture)
		glGenTextures(1, &colorTexture);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RayTracer::Render(const std::vector<DrawItem>& items, const FrameInfo& frame, const RenderSettings& settings,
	const Renderer& renderer, int width, int height)
{
	// ---- 1. World-space box of every traced item -------------------------------------------
	// A unit primitive lies in [-0.5, 0.5]^3, so under the model matrix M its box is centred on M's
	// translation with half-size |a0|/2 + |a1|/2 + |a2|/2 (a_k = the axis columns, component-wise abs).
	boxes.clear();
	order.clear();
	for (size_t i = 0; i < items.size(); ++i) {
		const DrawItem& item = items[i];
		if (item.material->opacity <= 0.01f)
			continue;
		const glm::vec3 c(item.model[3]);
		const glm::vec3 half = (glm::abs(glm::vec3(item.model[0])) + glm::abs(glm::vec3(item.model[1]))
			+ glm::abs(glm::vec3(item.model[2]))) * 0.5f + 1e-3f;
		boxes.push_back({c - half, c + half, c});
		order.push_back(static_cast<int>(i));
	}

	// ---- 2. Bounding volume hierarchy (rebuilt every frame: the toys move) --------------------
	// Top-down median split on the longest axis of the box centres; ~500 items -> ~250 leaves, depth ~9.
	nodes.clear();
	nodes.reserve(order.size() * 2);
	nodes.push_back({});
	if (!order.empty()) {
		// Build() sorts `order` and needs each item's box: index boxes by position in `order`.
		std::vector<int> slot(items.size(), -1);
		for (size_t k = 0; k < order.size(); ++k) slot[static_cast<size_t>(order[k])] = static_cast<int>(k);
		boxLookup = std::move(slot);
		Build(0, 0, static_cast<int>(order.size()));
	}

	// ---- 3. Pack: items in leaf order (8 texels each), then the nodes (2 texels each) ---------
	packed.clear();
	for (int idx : order) {
		const DrawItem& item = items[static_cast<size_t>(idx)];
		const Material& m = *item.material;
		// Rows 0..2 of the inverse model matrix. M = [A | t] is affine, so M^-1 = [A^-1 | -A^-1 t], and
		// A^-1 is the transpose of the normal matrix N = (A^-1)^T the renderer already computed:
		// row r of A^-1 = column r of N. No 4x4 inverse per item.
		const glm::vec3 t(item.model[3]);
		for (int r = 0; r < 3; ++r) {
			const glm::vec3 row = item.normalMatrix[r];
			packed.emplace_back(row, -glm::dot(row, t));
		}
		// A texture the tracer cannot sample (no slot) falls back to the material's plain colour.
		const bool sampled = settings.textures && (m.rtTextureSlot >= 0 || !m.texture);
		packed.emplace_back(m.DisplayColor(sampled), static_cast<float>(item.mesh->Type()));
		packed.emplace_back(m.ka, m.kd, m.ks, m.shininess);
		packed.emplace_back(m.emissive, m.opacity);
		packed.emplace_back(m.reflectivity, static_cast<float>(m.rtTextureSlot), m.uvScale.x, m.uvScale.y);
		packed.emplace_back(m.unlit ? 1.0f : 0.0f, m.cutout ? 1.0f : 0.0f, 0.0f, 0.0f);
	}
	instanceCount = static_cast<int>(order.size());
	const int nodeOffset = static_cast<int>(packed.size());
	for (const Node& n : nodes) {
		packed.emplace_back(n.lo, static_cast<float>(n.first));
		packed.emplace_back(n.hi, static_cast<float>(n.count));
	}
	if (order.empty()) { packed.emplace_back(glm::vec3(1.0f), 0.0f); packed.emplace_back(glm::vec3(-1.0f), 0.0f); } // empty root

	glBindBuffer(GL_TEXTURE_BUFFER, instanceBuffer);
	glBufferData(GL_TEXTURE_BUFFER, static_cast<GLsizeiptr>(std::max<size_t>(packed.size(), 1) * sizeof(glm::vec4)),
		packed.empty() ? nullptr : packed.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	// ---- 3. Trace at reduced resolution -------------------------------------------------------
	const int w = std::max(1, static_cast<int>(static_cast<float>(width) * settings.rayScale));
	const int h = std::max(1, static_cast<int>(static_cast<float>(height) * settings.rayScale));
	EnsureTarget(w, h);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, w, h);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	const Camera& cam = *frame.camera;
	shader.Activate();
	shader.SetVec3("uCamPos", cam.position);
	shader.SetVec3("uCamForward", cam.Forward());
	shader.SetVec3("uCamRight", cam.Right());
	shader.SetVec3("uCamUp", cam.Up());
	shader.SetFloat("uTanHalfFov", std::tan(glm::radians(cam.fov) * 0.5f));
	shader.SetFloat("uAspect", frame.aspect);
	shader.SetVec3("uBackground", frame.clearColor);
	shader.SetInt("uMaxBounces", settings.rayBounces);
	shader.SetInt("uUseTexture", settings.textures ? 1 : 0);
	shader.SetInt("uNodeOffset", nodeOffset);
	renderer.UploadLights(shader, frame, settings);
	for (size_t i = 0; i < frame.lights->size() && i < static_cast<size_t>(MaxLights); ++i)
		shader.SetInt("uLightShadow[" + std::to_string(i) + "]", (*frame.lights)[i].castsShadows ? 1 : 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_BUFFER, instanceTexture);
	shader.SetInt("uInstances", 0);
	for (int slot = 0; slot < Assets::SlotCount; ++slot) {
		assets->SlotTexture(slot)->Bind(static_cast<GLuint>(slot + 1));
		shader.SetInt("uTex" + std::to_string(slot), slot + 1);
	}

	glBindVertexArray(emptyVao);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// ---- 4. Upscale to the window -------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	presentShader.Activate();
	glActiveTexture(GL_TEXTURE0 + Assets::SlotCount + 1);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	presentShader.SetInt("uImage", Assets::SlotCount + 1);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}
