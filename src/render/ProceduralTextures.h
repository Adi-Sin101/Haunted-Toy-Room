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
Image Moon(int width = 1024, int height = 512);
Image Fabric(int pattern, int size = 256); // cotton, denim, leather, plaid, cow print
// House exterior (greyscale detail, tinted by the material colour):
Image Siding(int size = 256);         // horizontal lap boards with a shadow line under each board
Image Shingles(int size = 256);       // staggered rows of roof tabs
Image Brick(int size = 256);          // running-bond bricks with mortar joints
Image Grass(int size = 256);          // fine blades of lawn
// Textures that replace modelled detail (each used to be many separate shapes):
Image BookSpines(int size = 256);     // a row of 12 book spines of varied height and colour (was 6 cubes per shelf)
Image Pickets(int size = 256);        // white pickets + two rails with ALPHA = 0 between them (cut-out; was ~50 cubes)
Image WindowPane(int size = 128);     // glass with a white frame and cross mullions (was 4 cubes per window)
Image FlowerBed(int size = 256);      // leaves with coloured blossoms (was 7 spheres)

// Hash-based value noise in [0, 1], smoothly interpolated (used for wood grain, flicker, stars).
float ValueNoise(float x, float y);
float Hash(float x, float y);

}
