#pragma once

#include <array>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "DebugLines.h"
#include "RenderSettings.h"
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
	const Mesh* mesh = nullptr;
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
	int selectedOwner = -1;
	float time = 0.0f;
};

// Rasteriser: draws the flattened scene with the selected shading model.
//   1. opaque objects (depth test + back-face culling)
//   2. transparent objects, sorted far-to-near, alpha blended, depth writes off
//   3. debug overlay (selection gizmo, normals, vertex points)
class Renderer {
public:
	void Init(const Assets& assets);

	// Flattens the scene graph (world matrices must be up to date).
	void Collect(const SceneNode& root, const glm::vec3& cameraPos);
	const std::vector<DrawItem>& Items() const { return items; }

	void Render(const FrameInfo& frame, const RenderSettings& settings);
	void RenderDebug(const FrameInfo& frame, const RenderSettings& settings);

	DebugLines& Lines() { return lines; }

	// Uploads lights + illumination toggles to any shader that #includes lighting.glsl.
	void UploadLights(const Shader& shader, const FrameInfo& frame, const RenderSettings& settings) const;

private:
	void CollectNode(const SceneNode& node, const glm::vec3& cameraPos);
	void ApplyMaterial(const Shader& shader, const Material& m, const RenderSettings& settings) const;

	const Assets* assets = nullptr;
	Shader litShader;
	Shader gouraudShader;
	Shader debugShader;
	DebugLines lines;

	std::vector<DrawItem> items;
	std::vector<const DrawItem*> transparent;

	struct LightNames { std::string type, enabled, position, direction, color, intensity, constant, linear, quadratic, inner, outer; };
	std::array<LightNames, MaxLights> lightNames;
};
