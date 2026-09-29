#pragma once

#include <string>

// Shading models of Lecture 9.
enum class ShadingMode : int {
	Flat = 0,    // one normal per triangle
	Gouraud = 1, // lighting per vertex, colours interpolated
	Phong = 2,   // normals interpolated, lighting per pixel, Phong specular (R.V)
	Blinn = 3,   // as Phong but Blinn-Phong specular (N.H)
};

inline const char* ToString(ShadingMode m)
{
	switch (m) {
	case ShadingMode::Flat: return "Flat";
	case ShadingMode::Gouraud: return "Gouraud";
	case ShadingMode::Phong: return "Phong";
	case ShadingMode::Blinn: return "Blinn-Phong";
	}
	return "?";
}

// Every render toggle the user can flip at run time.
struct RenderSettings {
	ShadingMode shading = ShadingMode::Blinn;
	bool wireframe = false;
	bool lighting = false;
	bool shadingEnabled = false;
	bool textures = false;
	bool ambient = true;
	bool diffuse = true;
	bool specular = true;
	bool showNormals = false;
	bool showVertices = false;
	bool showGizmo = false;
	bool rayTracing = false;
	float rayScale = 0.5f;  // ray tracer resolution relative to the window
	int rayBounces = 2;     // reflection / transparency bounces
};
