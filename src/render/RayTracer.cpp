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
	// ---- 1. Group items per object (group 0 = static room, always tested) -------------------
	int maxOwner = -1;
	for (const DrawItem& item : items)
		maxOwner = std::max(maxOwner, item.ownerId);
	const int groupCount = std::min(maxOwner + 2, MaxGroups);
	if (static_cast<int>(buckets.size()) < groupCount)
		buckets.resize(static_cast<size_t>(groupCount));
	for (auto& b : buckets)
		b.clear();
	for (size_t i = 0; i < items.size(); ++i) {
		const DrawItem& item = items[i];
		if (item.material->opacity <= 0.01f)
			continue;
		const int g = std::clamp(item.ownerId + 1, 0, groupCount - 1);
		buckets[static_cast<size_t>(g)].push_back(static_cast<int>(i));
	}

	// ---- 2. Pack instances + compute each group's bounding sphere ----------------------------
	packed.clear();
	glm::vec4 groupSphere[MaxGroups];
	glm::ivec2 groupRange[MaxGroups];
	int instance = 0;
	for (int g = 0; g < groupCount; ++g) {
		const auto& bucket = buckets[static_cast<size_t>(g)];
		groupRange[g] = { instance, static_cast<int>(bucket.size()) };

		glm::vec3 lo(1e9f), hi(-1e9f);
		for (int idx : bucket) {
			const DrawItem& item = items[static_cast<size_t>(idx)];
			const Material& m = *item.material;
			const glm::mat4 inv = glm::inverse(item.model);

			// Rows 0..2 of the inverse matrix (row r = (inv[0][r], inv[1][r], inv[2][r], inv[3][r])).
			for (int r = 0; r < 3; ++r)
				packed.emplace_back(inv[0][r], inv[1][r], inv[2][r], inv[3][r]);
			packed.emplace_back(m.DisplayColor(settings.textures), static_cast<float>(item.mesh->Type()));
			packed.emplace_back(m.ka, m.kd, m.ks, m.shininess);
			packed.emplace_back(m.emissive, m.opacity);
			packed.emplace_back(m.reflectivity, static_cast<float>(m.rtTextureSlot), m.uvScale.x, m.uvScale.y);
			packed.emplace_back(m.unlit ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);

			// Bounding sphere of a unit primitive: centre = origin, radius <= sqrt(3)/2 * largest axis scale.
			const glm::vec3 c(item.model[3]);
			const float radius = 0.8661f * std::max({ glm::length(glm::vec3(item.model[0])),
				glm::length(glm::vec3(item.model[1])), glm::length(glm::vec3(item.model[2])) });
			lo = glm::min(lo, c - radius);
			hi = glm::max(hi, c + radius);
			++instance;
		}
		const glm::vec3 center = (lo + hi) * 0.5f;
		const float radius = bucket.empty() ? 0.0f : glm::length(hi - lo) * 0.5f;
		groupSphere[g] = g == 0 ? glm::vec4(0.0f, 0.0f, 0.0f, -1.0f) // -1 = always test
		                        : glm::vec4(center, radius);
	}
	instanceCount = instance;

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
	shader.SetInt("uGroupCount", groupCount);
	glUniform4fv(shader.Uniform("uGroupSphere[0]"), groupCount, &groupSphere[0].x);
	glUniform2iv(shader.Uniform("uGroupRange[0]"), groupCount, &groupRange[0].x);
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
