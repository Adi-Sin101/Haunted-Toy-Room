#pragma once

#include <string>

#include <glm/glm.hpp>

// Light sources (Lecture 8). The numeric values of LightType match the GLSL constants.
enum class LightType : int {
	Directional = 0, // parallel rays from a direction (sun, moon) - no attenuation
	Point = 1,       // radiates in all directions from a position, attenuated with distance
	Spot = 2,        // point light restricted to a cone (desk lamp shade, car headlights)
};

struct Light {
	std::string name;
	LightType type = LightType::Point;
	bool enabled = true;
	bool castsShadows = false;        // used by the ray tracer's shadow rays
	glm::vec3 position{ 0.0f };
	glm::vec3 direction{ 0.0f, -1.0f, 0.0f }; // direction the light travels (spot / directional)
	glm::vec3 color{ 1.0f };
	float intensity = 1.0f;

	// Attenuation: f(d) = 1 / (kc + kl d + kq d^2)
	float constant = 1.0f;
	float linear = 0.09f;
	float quadratic = 0.032f;

	// Spot cone, as cosines of the half-angles: full intensity inside inner, fades to 0 at outer.
	float innerCutoff = 0.95f;
	float outerCutoff = 0.85f;
};

constexpr int MaxLights = 8;
