#pragma once

#include <glm/glm.hpp>

// The procedural sky (shaders/sky.glsl) for one frame. Environment::Sky() fills it from the clock;
// the raster sky pass and the ray tracer upload it unchanged, so both paths draw the same sky.
struct SkyInfo {
	glm::vec3 sunDirection{ 0.0f, 1.0f, 0.0f };  // unit vectors TOWARD the sun and the moon
	glm::vec3 moonDirection{ 0.0f, 1.0f, 0.0f };
	glm::vec3 zenith{ 0.0f }, horizon{ 0.0f }, sunColor{ 1.0f };
	float daylight = 0.0f, sunsetGlow = 0.0f, stars = 1.0f;
};
