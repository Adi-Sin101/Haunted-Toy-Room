#pragma once

#include <glm/glm.hpp>

class Assets;
class SceneNode;
struct Material;

// Handles to the parts of the room that move or change at run time.
struct RoomRig {
	// Desk lamp: swivel (Y) -> arm tilt (X) -> head tilt (X) -> shade, bulb, light anchor
	SceneNode* lamp = nullptr;
	SceneNode* lampArm = nullptr;
	SceneNode* lampHead = nullptr;
	SceneNode* lampLightAnchor = nullptr;
	Material* bulbMaterial = nullptr;

	// Beach ball: root (position) -> shape (accumulated rolling rotation in basis)
	SceneNode* ball = nullptr;
	SceneNode* ballShape = nullptr;
	float ballRadius = 0.35f;

	// Sky seen through the window
	SceneNode* sun = nullptr;
	SceneNode* moon = nullptr;
	Material* skyMaterial = nullptr;
	glm::vec3 skyCenter{ 0.0f };

	// Ghost that haunts the room at night
	SceneNode* ghost = nullptr;
	SceneNode* ghostBody = nullptr;
	Material* ghostMaterial = nullptr;
};

// Room dimensions (world units). Floor is y = 0, walls are one-sided planes facing inward, so a camera
// outside the room looks straight through the near walls (back-face culling) like a doll's house.
namespace RoomSize {
constexpr float HalfWidth = 8.0f;   // x from -8 to 8
constexpr float HalfDepth = 6.0f;   // z from -6 to 6
constexpr float Height = 7.0f;
}

// Builds floor, walls with a window opening, ceiling, window frame, sky backdrop, sun, moon, rug,
// desk, desk lamp, poster, toy blocks, ball and ghost as children of `root`.
RoomRig BuildRoom(SceneNode& root, Assets& assets);
