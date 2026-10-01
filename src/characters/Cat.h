#pragma once

#include <array>

#include <glm/glm.hpp>

#include "Character.h"

class Assets;
class SceneNode;

// Penny: a white cat with ginger patches, built from spheres, cylinders and cones like the toys.
//
//   Root (paws on the floor, heading)
//   +-- Body (joint at shoulder height) .... torso, chest, haunch, two ginger patches on her left flank
//       +-- Head (joint) ... skull, muzzle, chin, ginger cap + eye patch, ears, nose, eyes, whiskers
//       +-- Tail (joint) -> TailTip (joint)   raised while walking, lies flat when asleep
//       +-- four leg joints -> leg + paw
//
// Poses blend smoothly: Walk (diagonal gait), Sit (haunches down, front legs straight, watching)
// and Sleep (lying on her right side, head down, eyes closed to thin dark lines, like the photos).
class Cat : public Character {
public:
	enum class Pose { Walk, Sit, Sleep };

	Cat(SceneNode& parent, Assets& assets, const glm::vec3& position, float headingDeg);
	void Animate(float dt, float time) override;

	void SetPose(Pose p) { pose = p; }
	Pose CurrentPose() const { return pose; }
	// Turns the head (within +-70 degrees) toward a world point; the body stays put.
	void LookAt(const glm::vec3& worldPoint) { lookTarget = worldPoint; looking = true; }
	void StopLooking() { looking = false; }
	// Body pitch for walking up or down a slope (stairs), in degrees, positive = nose up.
	void SetSlope(float degrees) { slope = degrees; }
	glm::vec3 HeadPosition() const;

	static constexpr float ShoulderHeight = 0.62f;

private:
	SceneNode* body = nullptr;
	SceneNode* head = nullptr;
	SceneNode* tail = nullptr;
	SceneNode* tailTip = nullptr;
	std::array<SceneNode*, 4> legs{};       // front left, front right, back left, back right
	std::array<SceneNode*, 4> openEyes{};   // two eyes + two pupils
	std::array<SceneNode*, 2> closedEyes{}; // thin dark lines shown while asleep or blinking
	Pose pose = Pose::Walk;
	float sitBlend = 0.0f, sleepBlend = 0.0f, slopeBlend = 0.0f, slope = 0.0f;
	float headYaw = 0.0f;
	glm::vec3 lookTarget{ 0.0f };
	bool looking = false;
};
