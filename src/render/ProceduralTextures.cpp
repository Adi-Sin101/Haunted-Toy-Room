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

Image Fabric(int pattern, int size)
{
	return Generate(size, size, [&](float u, float v) {
		const float grain = ValueNoise(u * 120.0f, v * 120.0f);
		const float weave = 0.5f + 0.5f * std::sin(u * 256.0f * glm::pi<float>()) * std::sin(v * 256.0f * glm::pi<float>());
		float shade = 0.78f + 0.15f * grain + 0.07f * weave;
		if (pattern == 1) shade *= 0.8f + 0.2f * std::abs(std::sin((u + v) * 160.0f));
		if (pattern == 2) shade = 0.65f + 0.3f * Fbm(u * 45.0f, v * 45.0f) + 0.1f * grain;
		if (pattern == 3) {
			const float x = std::abs(std::sin(u * 8.0f * glm::pi<float>()));
			const float y = std::abs(std::sin(v * 8.0f * glm::pi<float>()));
			return glm::mix(glm::vec3(0.5f, 0.3f, 0.15f), glm::vec3(shade), (x > 0.18f && y > 0.18f) ? 1.0f : 0.0f);
		}
		if (pattern == 4) {
			float spots = Fbm(u * 9.0f, v * 9.0f);
			return glm::vec3(spots > 0.51f ? 0.10f : shade);
		}
		return glm::vec3(shade);
	});
}

Image Moon(int width, int height)
{
	struct Crater { glm::vec3 center; float inverseRadius, rangeSquare; };
	std::vector<Crater> craters;
	for (int i = 0; i < 110; ++i) {
		const float seed = static_cast<float>(i);
		const float y = Hash(seed, 11.0f) * 2.0f - 1.0f, angle = Hash(seed, 29.0f) * glm::two_pi<float>();
		const float ring = std::sqrt(1.0f - y * y);
		const float radius = 0.018f + std::pow(Hash(seed, 53.0f), 2.0f) * 0.17f;
		craters.push_back({{ring * std::cos(angle), y, ring * std::sin(angle)}, 1.0f / radius, 2.25f * radius * radius});
	}
	return Generate(width, height, [&](float u, float v) {
		const float longitude = u * glm::two_pi<float>(), latitude = (v - 0.5f) * glm::pi<float>();
		const glm::vec3 n(std::cos(latitude) * std::sin(longitude), std::sin(latitude), std::cos(latitude) * std::cos(longitude));
		const float land = Fbm(n.x * 5.0f + n.z * 3.0f + 12.0f, n.y * 7.0f + 4.0f);
		float shade = 0.40f + land * 0.60f;
		shade += (ValueNoise(n.x * 180.0f + n.z * 40.0f, n.y * 180.0f) - 0.5f) * 0.10f;
		for (const Crater& crater : craters) {
			const float square = 2.0f - 2.0f * (n.x * crater.center.x + n.y * crater.center.y + n.z * crater.center.z);
			if (square > crater.rangeSquare) continue;
			const float d = std::sqrt(std::max(0.0f, square)) * crater.inverseRadius;
			shade -= 0.18f * std::exp(-d * d * 3.0f);
			shade += 0.20f * std::exp(-(d - 1.0f) * (d - 1.0f) * 70.0f);
			shade += (n.y - crater.center.y) * crater.inverseRadius * 0.16f * std::exp(-d * d * 2.0f);
		}
		// Baked sunlight exposes the relief, including a slight terminator on the eastern limb.
		const float lit = 0.32f + 0.68f * std::max(0.0f, glm::dot(n, glm::normalize(glm::vec3(-0.15f, 0.25f, 1.0f))));
		return glm::vec3(0.94f, 0.95f, 1.0f) * shade * lit;
	});
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

// 8 boards across v. Inside a board the shade ramps from light (top) to dark (bottom edge, where the
// next board overlaps it and casts a thin shadow), plus wood grain stretched along u.
Image Siding(int size)
{
	return Generate(size, size, [&](float u, float v) {
		const float boards = 8.0f;
		const float b = v * boards - std::floor(v * boards);       // 0 at a board's lower edge, 1 at its top
		float shade = 0.80f + 0.16f * b;                           // overlapping board: brighter towards its top
		if (b < 0.08f) shade *= 0.55f + 0.45f * (b / 0.08f);       // shadow line under the board above
		shade *= 0.94f + 0.06f * Fbm(u * 6.0f, v * 90.0f);         // grain along the board
		return glm::vec3(shade);
	});
}

// 8 rows of tabs; every other row is shifted by half a tab (staggered like real shingles).
Image Shingles(int size)
{
	return Generate(size, size, [&](float u, float v) {
		const float rows = 8.0f, tabs = 6.0f;
		const float row = std::floor(v * rows);
		const float rv = v * rows - row;
		const float su = u * tabs + (static_cast<int>(row) % 2 == 0 ? 0.0f : 0.5f);
		const float tab = std::floor(su);
		const float tu = su - tab;
		float shade = 0.72f + 0.22f * Hash(tab, row);              // each tab has its own tint
		shade *= 0.85f + 0.15f * rv;                               // lighter towards the exposed edge
		if (rv < 0.10f || tu < 0.04f || tu > 0.96f) shade *= 0.55f; // gaps between tabs and rows
		return glm::vec3(shade * (0.93f + 0.07f * ValueNoise(u * 120.0f, v * 120.0f)));
	});
}

// Running bond: 8 courses, 4 bricks per course, alternate courses offset by half a brick.
Image Brick(int size)
{
	return Generate(size, size, [&](float u, float v) {
		const float courses = 8.0f, perCourse = 4.0f;
		const float course = std::floor(v * courses);
		const float cv = v * courses - course;
		const float bu = u * perCourse + (static_cast<int>(course) % 2 == 0 ? 0.0f : 0.5f);
		const float brick = std::floor(bu);
		const float cu = bu - brick;
		if (cv < 0.12f || cu < 0.04f)
			return glm::vec3(1.25f);                               // pale mortar (tint 0.55 -> grey-beige)
		const float shade = 0.75f + 0.25f * Hash(brick, course) + 0.08f * (ValueNoise(u * 64.0f, v * 64.0f) - 0.5f);
		return glm::vec3(shade);
	});
}

// Lawn: two scales of noise for clumps and individual blades.
Image Grass(int size)
{
	return Generate(size, size, [&](float u, float v) {
		const float clumps = Fbm(u * 8.0f, v * 8.0f);
		const float blades = ValueNoise(u * 180.0f, v * 60.0f);
		return glm::vec3(0.62f + 0.30f * clumps + 0.18f * (blades - 0.5f));
	});
}

// 12 books across u. Book k has a random height h (fraction of the shelf), colour and a gold title band;
// above its top the shelf's dark back shows. Thin dark lines separate the spines.
Image BookSpines(int size)
{
	const glm::vec3 palette[6] = {{0.55f, 0.12f, 0.10f}, {0.10f, 0.22f, 0.50f}, {0.80f, 0.62f, 0.12f},
		{0.12f, 0.40f, 0.20f}, {0.45f, 0.16f, 0.45f}, {0.80f, 0.40f, 0.12f}};
	return Generate(size, size, [&](float u, float v) {
		const float books = 12.0f;
		const float k = std::floor(u * books);
		const float bu = u * books - k;
		const float h = 0.72f + 0.26f * Hash(k, 3.0f);
		if (v > h) return glm::vec3(0.10f, 0.07f, 0.05f);                      // shelf back above the book
		if (bu < 0.05f || bu > 0.95f) return glm::vec3(0.05f);                 // gap between spines
		glm::vec3 c = palette[static_cast<int>(Hash(k, 9.0f) * 6.0f) % 6];
		if (std::abs(v - (h - 0.12f)) < 0.025f) c = glm::vec3(0.90f, 0.75f, 0.35f); // gold title band
		const float round = 0.75f + 0.25f * std::sin(bu * glm::pi<float>());   // rounded spine shading
		return c * round * (0.92f + 0.08f * ValueNoise(u * 90.0f, v * 30.0f));
	});
}

// 8 pickets across u with pointed tops, two horizontal rails; everything else fully transparent.
Image Pickets(int size)
{
	Image img(size, size);
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			const float u = (static_cast<float>(x) + 0.5f) / size, v = (static_cast<float>(y) + 0.5f) / size;
			const float cell = u * 8.0f - std::floor(u * 8.0f);
			const float off = std::abs(cell - 0.5f);                            // distance from the picket's centre line
			const bool picket = off < 0.22f && v < 0.80f + 0.14f * (1.0f - off / 0.22f); // triangular tip
			const bool rail = (v > 0.24f && v < 0.33f) || (v > 0.60f && v < 0.69f);
			const float shade = picket ? 0.97f - 0.12f * (off / 0.22f) : 0.88f;
			img.Set(x, y, glm::vec3(shade), (picket || rail) ? 1.0f : 0.0f);
		}
	}
	return img;
}

Image WindowPane(int size)
{
	return Generate(size, size, [&](float u, float v) {
		const bool frame = u < 0.06f || u > 0.94f || v < 0.06f || v > 0.94f;
		const bool mullion = std::abs(u - 0.5f) < 0.03f || std::abs(v - 0.5f) < 0.03f;
		if (frame || mullion) return glm::vec3(0.95f, 0.95f, 0.93f);
		// glass: darker at the bottom, a soft diagonal sky reflection
		const float sheen = std::max(0.0f, 1.0f - std::abs((u + v) - 1.2f) * 4.0f) * 0.25f;
		return glm::mix(glm::vec3(0.16f, 0.24f, 0.36f), glm::vec3(0.32f, 0.44f, 0.60f), v) + glm::vec3(sheen);
	});
}

Image FlowerBed(int size)
{
	const glm::vec3 blossoms[4] = {{0.95f, 0.30f, 0.45f}, {0.98f, 0.95f, 0.95f}, {0.95f, 0.75f, 0.20f}, {0.70f, 0.35f, 0.85f}};
	return Generate(size, size, [&](float u, float v) {
		glm::vec3 c = glm::vec3(0.20f, 0.45f, 0.18f) * (0.75f + 0.5f * ValueNoise(u * 60.0f, v * 60.0f));
		const float cells = 10.0f;
		const float cx = std::floor(u * cells), cy = std::floor(v * cells);
		const glm::vec2 centre(Hash(cx, cy) * 0.6f + 0.2f, Hash(cy, cx + 3.0f) * 0.6f + 0.2f);
		const glm::vec2 local(u * cells - cx, v * cells - cy);
		if (glm::length(local - centre) < 0.28f)
			c = blossoms[static_cast<int>(Hash(cx + 1.0f, cy + 7.0f) * 4.0f) % 4];
		return c;
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
