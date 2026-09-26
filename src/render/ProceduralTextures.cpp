#include "ProceduralTextures.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>

namespace ProceduralTextures {

// Pseudo-random value in [0, 1) from a 2D integer lattice point (classic sine hash).
float Hash(float x, float y)
{
	const float h = std::sin(x * 127.1f + y * 311.7f) * 43758.5453f;
	return h - std::floor(h);
}

// Value noise: random values on integer lattice points, blended with the smoothstep curve.
float ValueNoise(float x, float y)
{
	const float ix = std::floor(x), iy = std::floor(y);
	const float fx = x - ix, fy = y - iy;
	const float ux = fx * fx * (3.0f - 2.0f * fx);
	const float uy = fy * fy * (3.0f - 2.0f * fy);
	const float a = Hash(ix, iy), b = Hash(ix + 1, iy);
	const float c = Hash(ix, iy + 1), d = Hash(ix + 1, iy + 1);
	return glm::mix(glm::mix(a, b, ux), glm::mix(c, d, ux), uy);
}

namespace {

// Fractal (layered) noise: sum of octaves with halving amplitude and doubling frequency.
float Fbm(float x, float y, int octaves = 4)
{
	float sum = 0.0f, amp = 0.5f;
	for (int i = 0; i < octaves; ++i) {
		sum += amp * ValueNoise(x, y);
		x *= 2.0f; y *= 2.0f; amp *= 0.5f;
	}
	return sum;
}

template <typename F>
Image Generate(int w, int h, F&& texel)
{
	Image img(w, h);
	for (int y = 0; y < h; ++y)
		for (int x = 0; x < w; ++x)
			img.Set(x, y, texel((static_cast<float>(x) + 0.5f) / w, (static_cast<float>(y) + 0.5f) / h));
	return img;
}

} // namespace

Image White()
{
	return Image(1, 1);
}

// Planks run along v. The texture holds 4 planks side by side (u) and each plank is split into
// 2 boards along v with an offset so the joints are staggered.
Image WoodFloor(int size)
{
	const glm::vec3 light(0.72f, 0.50f, 0.30f), dark(0.45f, 0.28f, 0.15f);
	return Generate(size, size, [&](float u, float v) {
		const float planks = 4.0f;
		const float plank = std::floor(u * planks);
		const float pu = u * planks - plank;                       // 0..1 across the plank
		const float boardShift = Hash(plank, 7.0f) * 0.5f;
		const float bv = v * 2.0f + boardShift;
		const float board = std::floor(bv);
		const float pv = bv - board;                               // 0..1 along the board

		// grain: stretched noise + sine rings distorted by noise
		const float n = Fbm(pu * 3.0f + plank * 13.0f, bv * 18.0f + board * 5.0f);
		const float rings = 0.5f + 0.5f * std::sin((pu * 10.0f + n * 6.0f) * glm::pi<float>());
		glm::vec3 c = glm::mix(dark, light, 0.35f + 0.5f * rings * n + 0.15f * Hash(plank, board));
		c *= 0.85f + 0.3f * Hash(plank + 3.0f, board + 1.0f);     // per-board tint

		// dark gaps at plank edges and board ends
		const float edge = std::min({ pu, 1.0f - pu, pv * 4.0f, (1.0f - pv) * 4.0f });
		if (edge < 0.02f)
			c *= 0.35f;
		return c;
	});
}

Image Wallpaper(int size)
{
	const glm::vec3 base(0.36f, 0.42f, 0.55f), stripe(0.30f, 0.35f, 0.47f), motif(0.75f, 0.70f, 0.50f);
	return Generate(size, size, [&](float u, float v) {
		const float su = u * 8.0f;
		glm::vec3 c = (static_cast<int>(std::floor(su)) % 2 == 0) ? base : stripe;
		// diamond |x| + |y| < r centred in every other stripe cell
		const float cx = su - std::floor(su) - 0.5f;
		const float cy = v * 8.0f - std::floor(v * 8.0f) - 0.5f;
		if (static_cast<int>(std::floor(su)) % 2 == 0 && std::abs(cx) + std::abs(cy) < 0.18f)
			c = motif;
		return c * (0.95f + 0.05f * ValueNoise(u * 64.0f, v * 64.0f));
	});
}

Image Checker(int size, int cells, const glm::vec3& a, const glm::vec3& b)
{
	return Generate(size, size, [&](float u, float v) {
		const int cu = static_cast<int>(std::floor(u * cells));
		const int cv = static_cast<int>(std::floor(v * cells));
		return ((cu + cv) % 2 == 0) ? a : b;
	});
}

// Each cell of a 64 x 32 grid may hold one star at a random position with random brightness.
Image NightSky(int width, int height)
{
	Image img(width, height);
	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
			img.Set(x, y, glm::vec3(0.0f));

	const int cellsX = 64, cellsY = 32;
	const int cw = width / cellsX, ch = height / cellsY;
	for (int cy = 0; cy < cellsY; ++cy) {
		for (int cx = 0; cx < cellsX; ++cx) {
			if (Hash(static_cast<float>(cx), static_cast<float>(cy)) > 0.55f)
				continue;
			const int sx = cx * cw + static_cast<int>(Hash(cx + 0.3f, cy + 0.7f) * (cw - 4)) + 2;
			const int sy = cy * ch + static_cast<int>(Hash(cx + 0.9f, cy + 0.1f) * (ch - 4)) + 2;
			const float brightness = 0.4f + 0.6f * Hash(cx + 5.0f, cy + 9.0f);
			const float radius = 0.8f + 1.4f * Hash(cx + 2.0f, cy + 4.0f);
			for (int dy = -2; dy <= 2; ++dy) {
				for (int dx = -2; dx <= 2; ++dx) {
					const float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
					const float f = std::max(0.0f, 1.0f - d / radius) * brightness;
					if (f > 0.0f)
						img.Set(sx + dx, sy + dy, glm::vec3(f, f, std::min(1.0f, f * 1.1f)));
				}
			}
		}
	}
	return img;
}

// u goes around the ball (longitude) -> 6 coloured gores; v near 0 or 1 is a pole -> white cap.
Image BeachBall(int width, int height)
{
	const glm::vec3 colors[6] = {
		{ 0.90f, 0.15f, 0.15f }, { 1.0f, 1.0f, 1.0f }, { 0.15f, 0.35f, 0.90f },
		{ 1.0f, 0.85f, 0.10f },  { 1.0f, 1.0f, 1.0f }, { 0.15f, 0.70f, 0.30f } };
	return Generate(width, height, [&](float u, float v) {
		if (v < 0.08f || v > 0.92f)
			return glm::vec3(1.0f);
		return colors[static_cast<int>(u * 6.0f) % 6];
	});
}

Image ToyBlock(int size, const glm::vec3& color)
{
	return Generate(size, size, [&](float u, float v) {
		const float x = u - 0.5f, y = v - 0.5f;
		const float border = std::max(std::abs(x), std::abs(y));
		if (border > 0.44f)
			return color * 0.55f;                     // darker bevel
		// five-pointed star: polar radius modulated by cos(5 * angle)
		const float r = std::sqrt(x * x + y * y);
		const float a = std::atan2(y, x);
		const float starRadius = 0.18f + 0.09f * std::cos(5.0f * a - glm::half_pi<float>());
		if (r < starRadius)
			return glm::vec3(1.0f, 0.95f, 0.8f);
		return color;
	});
}

Image Rug(int size)
{
	const glm::vec3 bands[4] = { { 0.55f, 0.12f, 0.15f }, { 0.85f, 0.70f, 0.35f }, { 0.20f, 0.30f, 0.50f }, { 0.85f, 0.70f, 0.35f } };
	return Generate(size, size, [&](float u, float v) {
		const float x = u - 0.5f, y = v - 0.5f;
		const float d = std::max(std::abs(x), std::abs(y)); // square rings
		const glm::vec3 c = bands[static_cast<int>(d * 16.0f) % 4];
		return c * (0.85f + 0.15f * ValueNoise(u * 90.0f, v * 90.0f));
	});
}

// Used only if assets/textures/poster.bmp cannot be loaded.
Image PosterFallback(int width, int height)
{
	return Generate(width, height, [&](float u, float v) {
		glm::vec3 c = glm::mix(glm::vec3(0.05f, 0.05f, 0.2f), glm::vec3(0.4f, 0.1f, 0.5f), v);
		const float dx = u - 0.5f, dy = v - 0.6f;
		if (dx * dx + dy * dy < 0.04f)
			c = glm::vec3(0.9f, 0.9f, 0.3f);
		if (u < 0.04f || u > 0.96f || v < 0.03f || v > 0.97f)
			c = glm::vec3(0.9f);
		return c;
	});
}

}
