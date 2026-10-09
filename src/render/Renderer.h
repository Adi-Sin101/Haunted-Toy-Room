#pragma once

#include <array>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "DebugLines.h"
#include "RenderSettings.h"
#include "SkyInfo.h"
#include "gl/Shader.h"
#include "scene/Light.h"

class Assets;
class Camera;
class Mesh;
class SceneNode;
struct Material;

// One mesh to draw with its final world matrix. The scene graph is flattened into this list once
// per frame; both the rasteriser and the ray tracer consume the same list.
struct DrawItem {
	const Mesh* mesh = nullptr;     // level of detail picked for the camera
	const Mesh* source = nullptr;   // full-detail mesh of the scene node
	glm::vec3 center{ 0.0f };       // bounding sphere (world space) used for frustum culling
	float radius = 0.0f;
	const Material* material = nullptr;
	glm::mat4 model{ 1.0f };
	glm::mat3 normalMatrix{ 1.0f };
	int ownerId = -1;
	float viewDepth = 0.0f; // distance to camera, for sorting transparent objects
};

// Per-frame data the renderer needs from the application.
struct FrameInfo {
	const Camera* camera = nullptr;
	float aspect = 1.0f;
	const std::vector<Light>* lights = nullptr;
	glm::vec3 ambientLight{ 0.1f };
	glm::vec3 clearColor{ 0.0f };
	SkyInfo sky;
	glm::vec3 fogColor{ 0.055f, 0.065f, 0.095f };
	float fogDensity=0.0f;
	float sunShadowStrength = 0.0f; // 0 indoors .. 1 outdoors: raster shadows of light 0 (sun / moon)
	int selectedOwner = -1;
	float time = 0.0f;
};

// What the last Render() call submitted to the GPU (shown in the title bar and by --benchmark).
struct RenderStats {
	int drawCalls = 0;
	int culled = 0; // shapes outside the camera frustum, never sent to the GPU
	long long triangles = 0;
	int shadowDrawCalls = 0;
	long long shadowTriangles = 0;
};

// Rasteriser: draws the flattened scene with the selected shading model.
//   1. opaque objects (depth test + back-face culling)
//   2. transparent objects, sorted far-to-near, alpha blended, depth writes off
//   3. debug overlay (selection gizmo, normals, vertex points)
class Renderer {
public:
	void Init(const Assets& assets);
	~Renderer();

	// Flattens the scene graph (world matrices must be up to date).
	void Collect(const SceneNode& root, const glm::vec3& cameraPos);
	const std::vector<DrawItem>& Items() const { return items; }

	void Render(const FrameInfo& frame, const RenderSettings& settings);
	void RenderDebug(const FrameInfo& frame, const RenderSettings& settings);

	DebugLines& Lines() { return lines; }
	const RenderStats& Stats() const { return stats; }

	// Uploads lights + illumination toggles to any shader that #includes lighting.glsl.
	void UploadLights(const Shader& shader, const FrameInfo& frame, const RenderSettings& settings) const;
	// Uploads the sky uniforms to any shader that #includes sky.glsl (sky pass, ray tracer).
	void UploadSky(const Shader& shader, const FrameInfo& frame) const;

	static constexpr int SunShadowSize = 2048;
	static constexpr float SunShadowHalfExtent = 30.0f; // the shadow map covers 60 x 60 units around the view

private:
	void CollectNode(const SceneNode& node, const glm::vec3& cameraPos);
	void RenderLampShadow(const FrameInfo& frame);
	void RenderSunShadow(const FrameInfo& frame);
	void RenderSky(const FrameInfo& frame);

	// Uniform locations used for every draw call, looked up once instead of by name per draw.
	struct DrawUniforms {
		GLint model = -1, normalMatrix = -1, highlight = -1;
		GLint color = -1, ka = -1, kd = -1, ks = -1, shininess = -1, emissive = -1, opacity = -1, unlit = -1, uvScale = -1, cutout = -1;
		void Resolve(const Shader& shader);
	};
	void ApplyMaterial(const DrawUniforms& u, const Material& m, const RenderSettings& settings) const;

	const Assets* assets = nullptr;
	DrawUniforms litUniforms, gouraudUniforms;
	GLint shadowModel = -1;
	Shader litShader;
	Shader gouraudShader;
	Shader debugShader;
	Shader shadowShader;
	Shader skyShader;
	GLuint shadowFbo = 0, shadowDepth = 0;
	GLuint sunShadowFbo = 0, sunShadowDepth = 0;
	GLuint skyVao = 0; // the sky's full-screen triangle is generated from gl_VertexID
	glm::mat4 lampViewProjection{1.0f};
	glm::mat4 sunViewProjection{1.0f};
	DebugLines lines;

	RenderStats stats;
	std::vector<DrawItem> items;
	std::vector<const DrawItem*> opaque, transparent;

	struct LightNames { std::string type, enabled, position, direction, color, intensity, constant, linear, quadratic, inner, outer; };
	std::array<LightNames, MaxLights> lightNames;
};
