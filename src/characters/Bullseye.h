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
	// L: a jump. The body joint (and so the saddle and a mounted rider) rises on a 0.9 s arc with
	// the legs tucked, while the root stays on the floor for the physics contact.
	void Special() override { Jump(); }
	const char* SpecialName() const override { return "jump"; }
	void Jump() { if (jumpTime < 0.0f) jumpTime = 0.0f; }
	bool Jumping() const { return jumpTime >= 0.0f; }
	float JumpLift() const { return jumpLift; }

	SceneNode* Seat() const { return seat; }
	static constexpr float JumpHeight = 0.75f, JumpDuration = 0.9f;

private:
	SceneNode* body = nullptr;
	SceneNode* neck = nullptr;
	SceneNode* head = nullptr;
	SceneNode* tail = nullptr;
	SceneNode* seat = nullptr;
	std::array<SceneNode*, 4> legs{}; // FL, FR, BL, BR
	float jumpTime = -1.0f, jumpLift = 0.0f;
};
