#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

// An RGBA8 image in CPU memory. Row 0 is the BOTTOM row, which is what glTexImage2D expects
// for texture coordinate v = 0.
struct Image {
	int width = 0;
	int height = 0;
	std::vector<std::uint8_t> pixels; // width * height * 4 bytes

	Image() = default;
	Image(int w, int h) : width(w), height(h), pixels(static_cast<size_t>(w) * h * 4, 255) {}

	void Set(int x, int y, const glm::vec3& rgb, float alpha = 1.0f)
	{
		const size_t i = (static_cast<size_t>(y) * width + x) * 4;
		const glm::vec3 c = glm::clamp(rgb, 0.0f, 1.0f) * 255.0f + 0.5f;
		pixels[i + 0] = static_cast<std::uint8_t>(c.r);
		pixels[i + 1] = static_cast<std::uint8_t>(c.g);
		pixels[i + 2] = static_cast<std::uint8_t>(c.b);
		pixels[i + 3] = static_cast<std::uint8_t>(glm::clamp(alpha, 0.0f, 1.0f) * 255.0f + 0.5f);
	}
};
