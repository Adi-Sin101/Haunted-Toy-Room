#include "Renderer.h"

#include <algorithm>
#include <cmath>

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

	for (int i = 0; i < MaxLights; ++i) {
		const std::string p = "uLights[" + std::to_string(i) + "].";
		lightNames[static_cast<size_t>(i)] = { p + "type", p + "enabled", p + "position", p + "direction", p + "color",
			p + "intensity", p + "constant", p + "linear", p + "quadratic", p + "innerCutoff", p + "outerCutoff" };
	}
	items.reserve(512);
	transparent.reserve(64);
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
	shader.SetInt("uUseAmbient", settings.ambient ? 1 : 0);
	shader.SetInt("uUseDiffuse", settings.diffuse ? 1 : 0);
	shader.SetInt("uUseSpecular", settings.specular ? 1 : 0);
	shader.SetInt("uBlinn", settings.shading == ShadingMode::Blinn ? 1 : 0);
}

void Renderer::ApplyMaterial(const Shader& shader, const Material& m, const RenderSettings& settings) const
{
	shader.SetVec3("uMaterial.color", m.color);
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

	const Shader& shader = settings.shading == ShadingMode::Gouraud ? gouraudShader : litShader;
	shader.Activate();
	shader.SetMat4("uView", view);
	shader.SetMat4("uProj", proj);
	shader.SetVec3("uCameraPos", camera.position);
	shader.SetInt("uShadingMode", static_cast<int>(settings.shading));
	shader.SetInt("uTexture", 0);
	shader.SetInt("uUseTexture", 1); // untextured materials sample a 1x1 white texture
	UploadLights(shader, frame, settings);

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
