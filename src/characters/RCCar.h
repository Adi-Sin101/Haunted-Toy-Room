#pragma once

#include <array>

#include <glm/glm.hpp>

#include "Character.h"

class Assets;
class SceneNode;
struct Material;

// Remote-controlled toy car.
//
//   Root (heading)
//   +-- Chassis ...... body, cabin, windows, spoiler, bumpers, headlight bulbs
//   |   +-- HeadlightL / HeadlightR (anchors used for the two spot lights)
//   +-- Wheel joints FL, FR (steer about Y) -> Spin joint (rolls about X) -> tyre, hub
//   +-- Wheel joints BL, BR                 -> Spin joint                  -> tyre, hub
//
// Wheel roll angle = distance / radius (radians), so the tyres roll without slipping.
class RCCar : public Character {
public:
	RCCar(SceneNode& parent, Assets& assets, const glm::vec3& position, float headingDeg);

	void Animate(float dt, float time) override;
	void Special() override { headlightsOn = !headlightsOn; }
	const char* SpecialName() const override { return "headlights"; }

	bool HeadlightsOn() const { return headlightsOn; }
	SceneNode* HeadlightAnchor(int i) const { return headlights[static_cast<size_t>(i)]; }
	float Steering() const { return steering; }

protected:
	float TurnFactor() const override;
	void OnDrive(const ControlInput& in) override { steerInput = in.turn; }

private:
	std::array<SceneNode*, 4> steerJoints{};
	std::array<SceneNode*, 4> spinJoints{};
	std::array<SceneNode*, 2> headlights{};
	Material* lampMaterial = nullptr;
	bool headlightsOn = true;
	float wheelAngle = 0.0f;
	float steering = 0.0f;
	float steerInput = 0.0f;
	static constexpr float WheelRadius = 0.22f;
};
