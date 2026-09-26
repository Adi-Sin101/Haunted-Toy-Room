#pragma once

#include <string>

#include <glm/glm.hpp>

class Texture;

// Surface description used by the Phong illumination model (Lecture 8):
//
//   I = ka * Ia * color  +  sum over lights [ kd * (N.L) * color + ks * (R.V)^n ] * Il * att  +  emissive
//
// color     : diffuse reflectance (object colour), multiplied by the texture colour
// ka/kd/ks  : ambient, diffuse and specular reflection coefficients
// shininess : specular exponent n (bigger = smaller, sharper highlight)
struct Material {
	std::string name;
	glm::vec3 color{ 0.8f };
	float ka = 1.0f;
	float kd = 1.0f;
	float ks = 0.3f;
	float shininess = 32.0f;
	glm::vec3 emissive{ 0.0f };   // light the surface gives off itself (bulb, sky, laser)
	float opacity = 1.0f;         // < 1 is drawn in the transparent pass with alpha blending
	float reflectivity = 0.0f;    // used by the ray tracer for mirror reflections
	bool unlit = false;           // true: output emissive + color*texture, ignore lights
	const Texture* texture = nullptr;
	glm::vec2 uvScale{ 1.0f };    // texture repeat count
	int rtTextureSlot = -1;       // which texture the ray tracer samples (-1 = none)
};
