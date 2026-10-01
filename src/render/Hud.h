#pragma once

#include <array>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "gl/Shader.h"

struct HudInfo {
	std::string clock, selection, description, controls, camera, rendering, status;
	int selected = -1;
	bool help = false, paused = false, edit = false, story = false, arrival = false;
	bool settingsOpen = false, lighting = false, shading = false, rayTracing = false, textures = false;
	bool mouseLook = false;
	float fps = 0.0f;
};

// Screen-space typography and controls; font coverage is generated with Windows GDI once.
// There is no UI library or external font download. Layout is independent of camera zoom.
class Hud {
public:
	void Init();
	~Hud();
	void Render(int width, int height, const HudInfo& info);
	int HitTest(glm::vec2 mouse, int windowWidth, int windowHeight, bool settingsOpen) const;
	bool Covers(glm::vec2 mouse, int windowWidth, int windowHeight, bool help, bool settingsOpen) const;
private:
	struct Glyph { glm::vec4 uv; float width = 0; };
	struct Font { GLuint texture = 0; std::array<Glyph, 95> glyphs{}; float height = 0; };
	struct Vertex { glm::vec2 p, uv; glm::vec4 color; };
	Font body, title;
	Shader shader;
	GLuint vao = 0, buffer = 0;
	std::vector<Vertex> bodyVertices, titleVertices;
	static Font MakeFont(const wchar_t* family, int pixels, int weight);
	void Rect(float x, float y, float w, float h, glm::vec4 color);
	void Text(const std::string& text, float x, float y, float size, glm::vec4 color,
		float maxWidth = 10000.0f, bool heading = false);
	static float Scale(int width, int height);
	void Draw(const std::vector<Vertex>& vertices, const Font& font);
};
