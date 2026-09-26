#pragma once

#include <array>

#include <glm/glm.hpp>

#include "Character.h"

class Assets;
class SceneNode;

// Bullseye, the toy horse.
//
//   Root (hooves on the floor, heading)
//   +-- Body (joint, bobs while galloping)
//       +-- Barrel (body ellipsoid), belly
//       +-- Saddle ........... saddle, horn
//       |   +-- Seat (joint)   <- Jessie is attached here while riding
//       +-- Neck (joint, tilted forward) -> neck, mane
//       |   +-- Head (joint, levelled)   -> skull, muzzle, eyes, ears, nostrils
//       +-- Tail (joint, swishes)
//       +-- FrontLeftHip / FrontRightHip / BackLeftHip / BackRightHip (joints) -> leg, hoof
class Bullseye : public Character {
public:
	Bullseye(SceneNode& parent, Assets& assets, const glm::vec3& position, float headingDeg);

	void Animate(float dt, float time) override;

	SceneNode* Seat() const { return seat; }

private:
	SceneNode* body = nullptr;
	SceneNode* neck = nullptr;
	SceneNode* head = nullptr;
	SceneNode* tail = nullptr;
	SceneNode* seat = nullptr;
	std::array<SceneNode*, 4> legs{}; // FL, FR, BL, BR
};
