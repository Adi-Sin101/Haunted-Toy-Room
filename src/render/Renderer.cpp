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

	// Sun / moon shadow map: same depth-only layout as the lamp's, filled with an orthographic view.
	glGenFramebuffers(1, &sunShadowFbo); glGenTextures(1, &sunShadowDepth);
	glBindTexture(GL_TEXTURE_2D, sunShadowDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SunShadowSize, SunShadowSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glBindFramebuffer(GL_FRAMEBUFFER, sunShadowFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sunShadowDepth, 0);
	glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Sun shadow framebuffer is incomplete");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	skyShader = Shader("shaders/sky.vert", "shaders/sky.frag");
	glGenVertexArrays(1, &skyVao);

	for (int i = 0; i < MaxLights; ++i) {
		const std::string p = "uLights[" + std::to_string(i) + "].";
		lightNames[static_cast<size_t>(i)] = { p + "type", p + "enabled", p + "position", p + "direction", p + "color",
			p + "intensity", p + "constant", p + "linear", p + "quadratic", p + "innerCutoff", p + "outerCutoff" };
	}
	litUniforms.Resolve(litShader);
	gouraudUniforms.Resolve(gouraudShader);
	shadowModel = shadowShader.Uniform("uModel");
	items.reserve(1024);
	opaque.reserve(1024);
	transparent.reserve(64);
}

Renderer::~Renderer()
{
	if (shadowDepth) glDeleteTextures(1, &shadowDepth);
	if (shadowFbo) glDeleteFramebuffers(1, &shadowFbo);
	if (sunShadowDepth) glDeleteTextures(1, &sunShadowDepth);
	if (sunShadowFbo) glDeleteFramebuffers(1, &sunShadowFbo);
	if (skyVao) glDeleteVertexArrays(1, &skyVao);
}

namespace {

// The six clipping planes of a view-projection matrix (Gribb & Hartmann): each plane is
// row3 +/- row0..2, stored as (normal, d) so that dot(normal, p) + d >= 0 means "inside".
struct Frustum {
	glm::vec4 planes[6];
	explicit Frustum(const glm::mat4& viewProj)
	{
		const glm::vec4 row0(viewProj[0][0], viewProj[1][0], viewProj[2][0], viewProj[3][0]);
		const glm::vec4 row1(viewProj[0][1], viewProj[1][1], viewProj[2][1], viewProj[3][1]);
		const glm::vec4 row2(viewProj[0][2], viewProj[1][2], viewProj[2][2], viewProj[3][2]);
		const glm::vec4 row3(viewProj[0][3], viewProj[1][3], viewProj[2][3], viewProj[3][3]);
		const glm::vec4 raw[6] = { row3 + row0, row3 - row0, row3 + row1, row3 - row1, row3 + row2, row3 - row2 };
		for (int i = 0; i < 6; ++i)
			planes[i] = raw[i] / glm::length(glm::vec3(raw[i]));
	}
	// A bounding sphere is culled only when it lies completely behind one plane.
	bool Visible(const glm::vec3& c, float r) const
	{
		for (const glm::vec4& p : planes)
			if (p.x * c.x + p.y * c.y + p.z * c.z + p.w < -r) return false;
		return true;
	}
};

} // namespace

void Renderer::DrawUniforms::Resolve(const Shader& shader)
{
	model = shader.Uniform("uModel");
	normalMatrix = shader.Uniform("uNormalMatrix");
	highlight = shader.Uniform("uHighlight");
	color = shader.Uniform("uMaterial.color");
	ka = shader.Uniform("uMaterial.ka");
	kd = shader.Uniform("uMaterial.kd");
	ks = shader.Uniform("uMaterial.ks");
	shininess = shader.Uniform("uMaterial.shininess");
	emissive = shader.Uniform("uMaterial.emissive");
	opacity = shader.Uniform("uMaterial.opacity");
	unlit = shader.Uniform("uMaterial.unlit");
	uvScale = shader.Uniform("uMaterial.uvScale");
	cutout = shader.Uniform("uCutout");
}

void Renderer::RenderLampShadow(const FrameInfo& frame)
{
	const Light& lamp = (*frame.lights)[2];
	const glm::vec3 up = std::abs(lamp.direction.y) > 0.95f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
	lampViewProjection = t3d::perspective(std::acos(std::clamp(lamp.outerCutoff, -1.0f, 1.0f)) * 2.0f, 1, 0.08f, 36.0f)
		* t3d::lookAt(lamp.position, lamp.position + lamp.direction, up);
	const Frustum lampFrustum(lampViewProjection);
	GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo); glViewport(0, 0, 2048, 2048);
	glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); glDisable(GL_BLEND); glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); glClear(GL_DEPTH_BUFFER_BIT);
	shadowShader.Activate(); shadowShader.SetMat4("uLightVP", lampViewProjection);
	for (const DrawItem& item : items) {
		if (item.material->unlit || item.material->opacity < 0.9f || item.material->cutout) continue;
		// Tiny details cast shadows smaller than a shadow-map texel; shapes outside the lamp's
		// cone cannot cast into it.
		if (item.radius < 0.035f || !lampFrustum.Visible(item.center, item.radius)) continue;
		const Mesh& mesh = item.source->ForScreenSize(item.radius / std::max(glm::distance(lamp.position, item.center), 0.01f));
		glUniformMatrix4fv(shadowModel, 1, GL_FALSE, &item.model[0][0]);
		mesh.Draw();
		++stats.shadowDrawCalls; stats.shadowTriangles += static_cast<long long>(mesh.TriangleCount());
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0); glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

// Directional light 0 (sun by day, moon by night) seen as parallel rays: an orthographic box,
// SunShadowHalfExtent wide, centred a little ahead of the camera and looking along the light.
void Renderer::RenderSunShadow(const FrameInfo& frame)
{
	const Light& sun = (*frame.lights)[0];
	const glm::vec3 dir = glm::normalize(sun.direction);
	const Camera& camera = *frame.camera;
	glm::vec3 ahead = camera.Forward(); ahead.y = 0.0f;
	ahead = glm::length(ahead) > 1e-3f ? glm::normalize(ahead) : glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 centre = camera.position + ahead * (SunShadowHalfExtent * 0.45f);
	// Snap the centre to whole shadow-map texels across the light's view: otherwise every camera
	// movement shifts the texel grid and the shadow edges crawl (shimmer).
	const glm::vec3 up = std::abs(dir.y) > 0.95f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
	const glm::vec3 right = glm::normalize(glm::cross(dir, up)), lightUp = glm::cross(right, dir);
	const float texel = SunShadowHalfExtent * 2.0f / static_cast<float>(SunShadowSize);
	centre += right * (std::floor(glm::dot(centre, right) / texel) * texel - glm::dot(centre, right))
		+ lightUp * (std::floor(glm::dot(centre, lightUp) / texel) * texel - glm::dot(centre, lightUp));
	constexpr float back = 90.0f;
	sunViewProjection = t3d::orthographic(SunShadowHalfExtent, SunShadowHalfExtent, 1.0f, back + 60.0f)
		* t3d::lookAt(centre - dir * back, centre, up);
	const Frustum sunFrustum(sunViewProjection);

	GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
	glBindFramebuffer(GL_FRAMEBUFFER, sunShadowFbo); glViewport(0, 0, SunShadowSize, SunShadowSize);
	glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); glDisable(GL_BLEND); glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); glClear(GL_DEPTH_BUFFER_BIT);
	shadowShader.Activate(); shadowShader.SetMat4("uLightVP", sunViewProjection);
	for (const DrawItem& item : items) {
		if (item.material->unlit || item.material->opacity < 0.9f || item.material->cutout) continue;
		// The lawn, street and pavement only RECEIVE shadows: as casters they add acne, not shade.
		if (item.radius < 0.05f || item.radius > 60.0f || !sunFrustum.Visible(item.center, item.radius)) continue;
		const Mesh& mesh = item.source->ForScreenSize(item.radius / (SunShadowHalfExtent * 0.5f));
		glUniformMatrix4fv(shadowModel, 1, GL_FALSE, &item.model[0][0]);
		mesh.Draw();
		++stats.shadowDrawCalls; stats.shadowTriangles += static_cast<long long>(mesh.TriangleCount());
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0); glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void Renderer::UploadSky(const Shader& shader, const FrameInfo& frame) const
{
	const SkyInfo& sky = frame.sky;
	shader.SetVec3("uSunDir", sky.sunDirection);
	shader.SetVec3("uMoonDir", sky.moonDirection);
	shader.SetVec3("uSkyZenith", sky.zenith);
	shader.SetVec3("uSkyHorizon", sky.horizon);
	shader.SetVec3("uSunColor", sky.sunColor);
	shader.SetFloat("uDaylight", sky.daylight);
	shader.SetFloat("uSunsetGlow", sky.sunsetGlow);
	shader.SetFloat("uStarAmount", sky.stars);
	shader.SetFloat("uSkyTime", frame.time);
}

// The sky fills every pixel the opaque pass left empty (depth still 1.0).
void Renderer::RenderSky(const FrameInfo& frame)
{
	const Camera& camera = *frame.camera;
	glm::mat4 rotation = camera.View();
	rotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // directions only: drop the translation
	skyShader.Activate();
	skyShader.SetMat4("uInvViewProj", glm::inverse(camera.Projection(frame.aspect) * rotation));
	UploadSky(skyShader, frame);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthFunc(GL_LEQUAL); glDepthMask(GL_FALSE); glDisable(GL_CULL_FACE);
	glBindVertexArray(skyVao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDepthFunc(GL_LESS); glDepthMask(GL_TRUE); glEnable(GL_CULL_FACE);
	++stats.drawCalls; stats.triangles += 1;
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
		item.source = node.mesh;
		item.material = node.material;
		item.model = node.World();
		item.normalMatrix = t3d::normalMatrix(item.model);
		item.ownerId = node.ownerId;
		item.center = node.WorldPosition();
		item.radius = t3d::unitBoundsRadius(item.model);
		item.viewDepth = glm::length(item.center - cameraPos);
		item.mesh = &node.mesh->ForScreenSize(item.radius / std::max(item.viewDepth, 0.01f));
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
	shader.SetVec3("uFogColor", frame.fogColor); // the sky's horizon colour: far ground melts into the sky
	shader.SetInt("uSunShadows", 0);             // raster passes switch it on after their shadow pass
	shader.SetFloat("uFogDensity",frame.fogDensity);
	shader.SetVec3("uAmbientLight", frame.ambientLight);
	shader.SetInt("uLightingEnabled", settings.lighting ? 1 : 0);
	shader.SetInt("uShadingEnabled", settings.shadingEnabled ? 1 : 0);
	shader.SetInt("uUseAmbient", settings.ambient ? 1 : 0);
	shader.SetInt("uUseDiffuse", settings.diffuse ? 1 : 0);
	shader.SetInt("uUseSpecular", settings.specular ? 1 : 0);
	shader.SetInt("uBlinn", settings.shading == ShadingMode::Blinn ? 1 : 0);
	shader.SetInt("uRasterShadows", 0); // analytic visibility is used by the ray tracer
}

void Renderer::ApplyMaterial(const DrawUniforms& u, const Material& m, const RenderSettings& settings) const
{
	const glm::vec3 color = m.DisplayColor(settings.textures);
	glUniform3f(u.color, color.r, color.g, color.b);
	glUniform1f(u.ka, m.ka);
	glUniform1f(u.kd, m.kd);
	glUniform1f(u.ks, m.ks);
	glUniform1f(u.shininess, m.shininess);
	glUniform3f(u.emissive, m.emissive.r, m.emissive.g, m.emissive.b);
	glUniform1f(u.opacity, m.opacity);
	glUniform1i(u.unlit, m.unlit ? 1 : 0);
	glUniform2f(u.uvScale, m.uvScale.x, m.uvScale.y);
	glUniform1i(u.cutout, m.cutout ? 1 : 0);

	// A cut-out's alpha is its SHAPE, so its texture is bound even when textures are switched off.
	const Texture* tex = ((settings.textures || m.cutout) && m.texture) ? m.texture : &assets->WhiteTexture();
	tex->Bind(0);
}

void Renderer::Render(const FrameInfo& frame, const RenderSettings& settings)
{
	stats = {};
	const Light& lamp = (*frame.lights)[2];
	// The shadow map only matters while the lamp is shining: skip the whole pass otherwise.
	const bool lampShadows = settings.lighting && settings.shadingEnabled && lamp.enabled && lamp.intensity > 0.001f;
	if (lampShadows) RenderLampShadow(frame);
	// Sun / moon shadows only outdoors, and only while that light actually shines.
	const Light& sun = (*frame.lights)[0];
	const bool sunShadows = settings.lighting && settings.shadingEnabled && sun.enabled && sun.intensity > 0.01f
		&& frame.sunShadowStrength > 0.01f;
	if (sunShadows) RenderSunShadow(frame);
	const Camera& camera = *frame.camera;
	const glm::mat4 view = camera.View();
	const glm::mat4 proj = camera.Projection(frame.aspect);
	const Frustum frustum(proj * view);

	glClearColor(frame.clearColor.r, frame.clearColor.g, frame.clearColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glPolygonMode(GL_FRONT_AND_BACK, settings.wireframe ? GL_LINE : GL_FILL);

	const bool gouraud = settings.shadingEnabled && settings.shading == ShadingMode::Gouraud;
	const Shader& shader = gouraud ? gouraudShader : litShader;
	const DrawUniforms& u = gouraud ? gouraudUniforms : litUniforms;
	shader.Activate();
	shader.SetMat4("uView", view);
	shader.SetMat4("uProj", proj);
	shader.SetVec3("uCameraPos", camera.position);
	shader.SetInt("uShadingMode", static_cast<int>(settings.shading));
	shader.SetInt("uTexture", 0);
	shader.SetInt("uUseTexture", settings.textures ? 1 : 0);
	UploadLights(shader, frame, settings);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, shadowDepth);
	shader.SetInt("uShadowMap", 1); shader.SetInt("uRasterShadows", lampShadows ? 1 : 0);
	shader.SetMat4("uLightVP", lampViewProjection);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, sunShadowDepth); glActiveTexture(GL_TEXTURE0);
	shader.SetInt("uSunShadowMap", 2); shader.SetInt("uSunShadows", sunShadows ? 1 : 0);
	shader.SetFloat("uSunShadowStrength", frame.sunShadowStrength);
	shader.SetMat4("uSunVP", sunViewProjection);

	const float pulse = 0.55f + 0.45f * std::sin(frame.time * 5.0f);
	const Material* currentMaterial = nullptr;
	const Mesh* currentMesh = nullptr;
	float currentHighlight = -1.0f;
	GLenum currentWinding = GL_CCW;
	auto draw = [&](const DrawItem& item) {
		const GLenum winding = glm::determinant(glm::mat3(item.model)) < 0.0f ? GL_CW : GL_CCW;
		if (winding != currentWinding) { glFrontFace(winding); currentWinding = winding; }
		if (item.material != currentMaterial) { // skip redundant material uploads
			ApplyMaterial(u, *item.material, settings);
			currentMaterial = item.material;
		}
		if (item.mesh != currentMesh) { // consecutive shapes often share a mesh: bind its VAO once
			item.mesh->Bind();
			currentMesh = item.mesh;
		}
		glUniformMatrix4fv(u.model, 1, GL_FALSE, &item.model[0][0]);
		glUniformMatrix3fv(u.normalMatrix, 1, GL_FALSE, &item.normalMatrix[0][0]);
		const float highlight = (frame.selectedOwner >= 0 && item.ownerId == frame.selectedOwner) ? pulse : 0.0f;
		if (highlight != currentHighlight) { glUniform1f(u.highlight, highlight); currentHighlight = highlight; }
		item.mesh->DrawBound();
		++stats.drawCalls; stats.triangles += static_cast<long long>(item.mesh->TriangleCount());
	};

	// Frustum culling: shapes behind the camera or off-screen are never submitted.
	opaque.clear();
	transparent.clear();
	for (const DrawItem& item : items) {
		if (!frustum.Visible(item.center, item.radius)) { ++stats.culled; continue; }
		(item.material->opacity < 1.0f ? transparent : opaque).push_back(&item);
	}

	// Pass 1: opaque, front to back. A fragment hidden behind something already drawn fails the depth
	// test BEFORE its fragment shader runs (early-z), so near objects drawn first save the lighting work
	// of everything behind them. Within 0.5-unit depth slices, items are grouped by material and mesh to
	// keep state changes low.
	std::sort(opaque.begin(), opaque.end(), [](const DrawItem* a, const DrawItem* b) {
		const int sa = static_cast<int>(a->viewDepth * 2.0f), sb = static_cast<int>(b->viewDepth * 2.0f);
		if (sa != sb) return sa < sb;
		return a->material != b->material ? a->material < b->material : a->mesh < b->mesh;
	});
	for (const DrawItem* item : opaque)
		draw(*item);

	// The sky after the opaque pass (only uncovered pixels run its shader) and before the transparent
	// one (glass and the ghost blend over the sky).
	RenderSky(frame);
	shader.Activate();
	currentMesh = nullptr;
	glPolygonMode(GL_FRONT_AND_BACK, settings.wireframe ? GL_LINE : GL_FILL);
	glFrontFace(currentWinding);

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
	glFrontFace(GL_CCW);
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
