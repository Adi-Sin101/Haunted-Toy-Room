#pragma once

#include <string>

#include <glm/glm.hpp>

#include "Character.h"

class Assets;
class SceneNode;
struct Material;

// Colours and options that turn the ONE humanoid builder into Woody, Jessie or Buzz.
struct HumanoidStyle {
	std::string name;
	glm::vec3 skin{ 0.96f, 0.80f, 0.66f };
	glm::vec3 shirt{ 1.0f };
	glm::vec3 vest{ 1.0f };
	glm::vec3 pants{ 0.2f, 0.3f, 0.7f };
	glm::vec3 boots{ 0.4f, 0.22f, 0.1f };
	glm::vec3 hands{ 0.96f, 0.80f, 0.66f };
	glm::vec3 hair{ 0.35f, 0.2f, 0.1f };
	glm::vec3 hat{ 0.5f, 0.3f, 0.15f };
	glm::vec3 belt{ 0.25f, 0.15f, 0.08f };
	bool hasVest = true;
	bool hasHat = true;
	bool longHair = false;   // Jessie's braid
	bool spaceRanger = false; // Buzz: hood, helmet, chest plate, wings, laser, no hat/hair
};

// Walking, bipedal toy built from cylinders, spheres and cubes.
//
// Hierarchy (every child is positioned relative to its parent):
//
//   Root (feet, heading)
//   +-- Pelvis ........ belt, buckle
//   |   +-- Torso ..... chest, vest panels, neck
//   |   |   +-- Head (joint) ... head, eyes, pupils, nose, mouth, hair / braid, hat
//   |   |   +-- LeftShoulder (joint)  -> upper arm, hand
//   |   |   +-- RightShoulder (joint) -> upper arm, hand
//   |   +-- LeftHip (joint)  -> leg, boot, foot
//   |   +-- RightHip (joint) -> leg, boot, foot
class Humanoid : public Character {
public:
	Humanoid(SceneNode& parent, Assets& assets, const HumanoidStyle& style, const glm::vec3& position, float headingDeg);

	void Animate(float dt, float time) override;

	// Riding pose (legs astride, hands forward on the reins).
	void SetSeated(bool value) { seated = value; }
	bool Seated() const { return seated; }

	static constexpr float HipHeight = 0.85f;

protected:
	SceneNode* torso = nullptr;
	SceneNode* head = nullptr;
	SceneNode* leftShoulder = nullptr;
	SceneNode* rightShoulder = nullptr;
	SceneNode* leftHip = nullptr;
	SceneNode* rightHip = nullptr;
	SceneNode* rightHand = nullptr;

	bool seated = false;
	float seatBlend = 0.0f;
	float rightArmOverride = 0.0f; // Buzz raises his arm to fire the laser (blend 0..1)
};

// Buzz Lightyear: humanoid + wings that open while flying + wrist laser.
class Buzz : public Humanoid {
public:
	Buzz(SceneNode& parent, Assets& assets, const glm::vec3& position, float headingDeg);

	void Animate(float dt, float time) override;
	void Special() override { laserOn = !laserOn; }
	const char* SpecialName() const override { return "laser"; }

	bool LaserOn() const { return laserOn; }
	glm::vec3 LaserTip() const;

private:
	SceneNode* wings = nullptr;
	SceneNode* laserBeam = nullptr;
	SceneNode* laserTip = nullptr;
	bool laserOn = false;
	float wingOpen = 0.0f;
};

HumanoidStyle WoodyStyle();
HumanoidStyle JessieStyle();
