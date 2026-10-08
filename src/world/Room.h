#pragma once

#include <glm/glm.hpp>
#include <vector>

class Assets;
class SceneNode;
struct Material;

// Handles to the parts of the room that move or change at run time.
struct RoomRig {
	std::vector<SceneNode*> blocks;
	SceneNode* missionObstacle = nullptr;
	SceneNode* fanRotor = nullptr;
	SceneNode* clockHour = nullptr;
	SceneNode* clockMinute = nullptr;
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
	Material* skylineMaterial = nullptr; // distant houses, recoloured with the daylight
	glm::vec3 skyCenter{ 0.0f };

	// Ghost that haunts the room at night
	SceneNode* ghost = nullptr;
	SceneNode* ghostBody = nullptr;
	Material* ghostMaterial = nullptr;
};

// Room dimensions (world units). Floor is y = 0, walls are one-sided planes facing inward, so a camera
// outside the room looks straight through the near walls (back-face culling) like a doll's house.
namespace RoomSize {
constexpr float HalfWidth = 10.0f;
constexpr float HalfDepth = 9.0f;
constexpr float Height = 7.5f;
constexpr float HallEnd = 17.0f;
constexpr float DoorLow = 1.0f, DoorHigh = 7.0f, DoorHeight = 4.8f;
// The toy room is the upper floor of a house. The ground floor lies below it: a corridor runs from the
// front door to a staircase that climbs (+Z) into the hallway through an opening in its z = DoorLow wall.
constexpr float Ground = -4.5f;                          // ground-floor level and the garden lawn
constexpr float Slab = -0.3f;                            // underside of the upper floor = ground-floor ceiling
constexpr float CorridorLeft = 10.5f, CorridorRight = 13.5f;
constexpr float StairLeft = 13.5f, StairRight = 16.5f;
constexpr float StairBottomZ = -7.0f, StairTopZ = 1.0f;  // the flight rises 4.5 over a run of 8
constexpr float HouseBack = -9.35f, HouseFront = 9.35f;  // outer faces of the exterior walls
constexpr float HouseLeft = -10.45f, HouseRight = 17.45f;
constexpr float FrontDoorLeft = 11.2f, FrontDoorRight = 12.8f, FrontDoorHeight = 3.0f;
}

// Builds floor, walls with a window opening, ceiling, window frame, sky backdrop, sun, moon, rug,
// desk, desk lamp, poster, toy blocks, ball and ghost as children of `root`.
RoomRig BuildRoom(SceneNode& root, Assets& assets);
