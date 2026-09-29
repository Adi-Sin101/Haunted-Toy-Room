#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <glad/glad.h>

#include "Assets.h"
#include "geometry/Mesh.h"
#include "math/Transform3D.h"
#include "scene/Camera.h"
#include "scene/Material.h"
#include "scene/SceneNode.h"

void Renderer::Init(const Assets& library)
{
	assets = &library;
	litShader = Shader("shaders/lit.vert", "shaders/lit.frag");
	gouraudShader = Shader("shaders/gouraud.vert", "shaders/gouraud.frag");
	debugShader = Shader("shaders/debug.vert", "shaders/debug.frag");
	shadowShader = Shader("shaders/shadow.vert", "shaders/shadow.frag");
	glGenFramebuffers(1, &shadowFbo); glGenTextures(1, &shadowDepth);
	glBindTexture(GL_TEXTURE_2D, shadowDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	const float border[] = {1, 1, 1, 1}; glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepth, 0);
	glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Lamp shadow framebuffer is incomplete");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	for (int i = 0; i < MaxLights; ++i) {
		const std::string p = "uLights[" + std::to_string(i) + "].";
		lightNames[static_cast<size_t>(i)] = { p + "type", p + "enabled", p + "position", p + "direction", p + "color",
			p + "intensity", p + "constant", p + "linear", p + "quadratic", p + "innerCutoff", p + "outerCutoff" };
	}
	items.reserve(512);
	transparent.reserve(64);
}

Renderer::~Renderer()
{
	if (shadowDepth) glDeleteTextures(1, &shadowDepth);
	if (shadowFbo) glDeleteFramebuffers(1, &shadowFbo);
}

void Renderer::RenderLampShadow(const FrameInfo& frame)
{
	const Light& lamp = (*frame.lights)[2];
	const glm::vec3 up = std::abs(lamp.direction.y) > 0.95f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
	lampViewProjection = t3d::perspective(std::acos(std::clamp(lamp.outerCutoff, -1.0f, 1.0f)) * 2.0f, 1, 0.08f, 36.0f)
		* t3d::lookAt(lamp.position, lamp.position + lamp.direction, up);
	GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo); glViewport(0, 0, 2048, 2048);
	glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); glDisable(GL_BLEND); glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); glClear(GL_DEPTH_BUFFER_BIT);
	shadowShader.Activate(); shadowShader.SetMat4("uLightVP", lampViewProjection);
	for (const DrawItem& item : items) {
		if (item.material->unlit || item.material->opacity < 0.9f) continue;
		shadowShader.SetMat4("uModel", item.model); item.mesh->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0); glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void Renderer::Collect(const SceneNode& root, const glm::vec3& cameraPos)
{
	items.clear();
	CollectNode(root, cameraPos);
}

void Renderer::CollectNode(const SceneNode& node, const glm::vec3& cameraPos)
{
	if (!node.visible)
		return;
	if (node.mesh && node.material) {
		DrawItem item;
		item.mesh = node.mesh;
		item.material = node.material;
		item.model = node.World();
		item.normalMatrix = t3d::normalMatrix(item.model);
		item.ownerId = node.ownerId;
		item.viewDepth = glm::length(node.WorldPosition() - cameraPos);
		items.push_back(item);
	}
	for (const auto& child : node.Children())
		CollectNode(*child, cameraPos);
}

void Renderer::UploadLights(const Shader& shader, const FrameInfo& frame, const RenderSettings& settings) const
{
	const auto& lights = *frame.lights;
	const int count = std::min(static_cast<int>(lights.size()), MaxLights);
	glUniform1i(shader.Uniform("uLightCount"), count);
	for (int i = 0; i < count; ++i) {
		const Light& l = lights[static_cast<size_t>(i)];
		const LightNames& n = lightNames[static_cast<size_t>(i)];
		shader.SetInt(n.type, static_cast<int>(l.type));
		shader.SetInt(n.enabled, l.enabled ? 1 : 0);
		shader.SetVec3(n.position, l.position);
		shader.SetVec3(n.direction, glm::normalize(l.direction));
		shader.SetVec3(n.color, l.color);
		shader.SetFloat(n.intensity, l.intensity);
		shader.SetFloat(n.constant, l.constant);
		shader.SetFloat(n.linear, l.linear);
		shader.SetFloat(n.quadratic, l.quadratic);
		shader.SetFloat(n.inner, l.innerCutoff);
		shader.SetFloat(n.outer, l.outerCutoff);
	}
	shader.SetVec3("uAmbientLight", frame.ambientLight);
	shader.SetInt("uLightingEnabled", settings.lighting ? 1 : 0);
	shader.SetInt("uShadingEnabled", settings.shadingEnabled ? 1 : 0);
	shader.SetInt("uUseAmbient", settings.ambient ? 1 : 0);
	shader.SetInt("uUseDiffuse", settings.diffuse ? 1 : 0);
	shader.SetInt("uUseSpecular", settings.specular ? 1 : 0);
	shader.SetInt("uBlinn", settings.shading == ShadingMode::Blinn ? 1 : 0);
	shader.SetInt("uRasterShadows", 0); // analytic visibility is used by the ray tracer
}

void Renderer::ApplyMaterial(const Shader& shader, const Material& m, const RenderSettings& settings) const
{
	shader.SetVec3("uMaterial.color", m.DisplayColor(settings.textures));
	shader.SetFloat("uMaterial.ka", m.ka);
	shader.SetFloat("uMaterial.kd", m.kd);
	shader.SetFloat("uMaterial.ks", m.ks);
	shader.SetFloat("uMaterial.shininess", m.shininess);
	shader.SetVec3("uMaterial.emissive", m.emissive);
	shader.SetFloat("uMaterial.opacity", m.opacity);
	shader.SetInt("uMaterial.unlit", m.unlit ? 1 : 0);
	shader.SetVec2("uMaterial.uvScale", m.uvScale);

	const Texture* tex = (settings.textures && m.texture) ? m.texture : &assets->WhiteTexture();
	tex->Bind(0);
}

void Renderer::Render(const FrameInfo& frame, const RenderSettings& settings)
{
	if (settings.lighting && settings.shadingEnabled) RenderLampShadow(frame);
	const Camera& camera = *frame.camera;
	const glm::mat4 view = camera.View();
	const glm::mat4 proj = camera.Projection(frame.aspect);

	glClearColor(frame.clearColor.r, frame.clearColor.g, frame.clearColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glPolygonMode(GL_FRONT_AND_BACK, settings.wireframe ? GL_LINE : GL_FILL);

	const Shader& shader = settings.shadingEnabled && settings.shading == ShadingMode::Gouraud ? gouraudShader : litShader;
	shader.Activate();
	shader.SetMat4("uView", view);
	shader.SetMat4("uProj", proj);
	shader.SetVec3("uCameraPos", camera.position);
	shader.SetInt("uShadingMode", static_cast<int>(settings.shading));
	shader.SetInt("uTexture", 0);
	shader.SetInt("uUseTexture", settings.textures ? 1 : 0);
	UploadLights(shader, frame, settings);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, shadowDepth);
	shader.SetInt("uShadowMap", 1); shader.SetInt("uRasterShadows", settings.lighting && settings.shadingEnabled ? 1 : 0);
	shader.SetMat4("uLightVP", lampViewProjection);

	const float pulse = 0.55f + 0.45f * std::sin(frame.time * 5.0f);
	const Material* current = nullptr;
	auto draw = [&](const DrawItem& item) {
		if (item.material != current) { // skip redundant material uploads
			ApplyMaterial(shader, *item.material, settings);
			current = item.material;
		}
		shader.SetMat4("uModel", item.model);
		shader.SetMat3("uNormalMatrix", item.normalMatrix);
		shader.SetFloat("uHighlight", (frame.selectedOwner >= 0 && item.ownerId == frame.selectedOwner) ? pulse : 0.0f);
		item.mesh->Draw();
	};

	// Pass 1: opaque
	transparent.clear();
	for (const DrawItem& item : items) {
		if (item.material->opacity < 1.0f)
			transparent.push_back(&item);
		else
			draw(item);
	}

	// Pass 2: transparent, far to near, blended over what is already drawn
	if (!transparent.empty()) {
		std::sort(transparent.begin(), transparent.end(),
			[](const DrawItem* a, const DrawItem* b) { return a->viewDepth > b->viewDepth; });
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		for (const DrawItem* item : transparent)
			draw(*item);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(0);
}

void Renderer::RenderDebug(const FrameInfo& frame, const RenderSettings& settings)
{
	const Camera& camera = *frame.camera;
	const glm::mat4 viewProj = camera.Projection(frame.aspect) * camera.View();

	if (settings.showNormals && frame.selectedOwner >= 0) {
		for (const DrawItem& item : items) {
			if (item.ownerId != frame.selectedOwner)
				continue;
			for (const Vertex& v : item.mesh->Data().vertices) {
				const glm::vec3 p = glm::vec3(item.model * glm::vec4(v.position, 1.0f));
				const glm::vec3 n = glm::normalize(item.normalMatrix * v.normal);
				lines.Add(p, p + n * 0.08f, glm::vec3(0.2f, 0.9f, 1.0f));
			}
		}
	}

	debugShader.Activate();
	debugShader.SetMat4("uViewProj", viewProj);

	if (!lines.Empty()) {
		debugShader.SetMat4("uModel", glm::mat4(1.0f));
		debugShader.SetInt("uUseVertexColor", 1);
		lines.Draw();
	}

	if (settings.showVertices && frame.selectedOwner >= 0) {
		glEnable(GL_PROGRAM_POINT_SIZE);
		glDisable(GL_DEPTH_TEST); // show hidden vertices too
		debugShader.SetInt("uUseVertexColor", 0);
		debugShader.SetVec3("uColor", glm::vec3(1.0f, 0.2f, 0.6f));
		debugShader.SetFloat("uPointSize", 4.0f);
		for (const DrawItem& item : items) {
			if (item.ownerId != frame.selectedOwner)
				continue;
			debugShader.SetMat4("uModel", item.model);
			item.mesh->DrawPoints();
		}
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_PROGRAM_POINT_SIZE);
	}
	lines.Clear();
	glBindVertexArray(0);
}
