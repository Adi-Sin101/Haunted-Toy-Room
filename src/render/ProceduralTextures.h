#pragma once

#include <glm/glm.hpp>

#include "Image.h"

// Textures computed texel-by-texel on the CPU (no image files needed).
// Every generator works in normalised texture space (u, v) in [0, 1) so the maths reads the same
// as the UV mapping of the meshes.
namespace ProceduralTextures {

Image White();
Image WoodFloor(int size = 512);      // planks with per-plank tint, grain and gaps
Image Wallpaper(int size = 256);      // two-tone vertical stripes with small diamonds
Image Checker(int size, int cells, const glm::vec3& a, const glm::vec3& b);
Image NightSky(int width = 1024, int height = 512); // stars on black (added on top of the sky colour)
Image BeachBall(int width = 256, int height = 128);  // coloured gores meeting at white poles
Image ToyBlock(int size, const glm::vec3& color);   // bevelled face with a star emblem
Image Rug(int size = 256);            // concentric bands
Image PosterFallback(int width = 256, int height = 384);

// Hash-based value noise in [0, 1], smoothly interpolated (used for wood grain, flicker, stars).
float ValueNoise(float x, float y);
float Hash(float x, float y);

}
