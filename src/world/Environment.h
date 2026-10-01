#pragma once

#include <string>

#include <glm/glm.hpp>

#include "Room.h"

// The world's own motion, independent of the user:
//   * clock + day/night cycle: sun and moon travel across the window, sky/ambient colours blend
//   * haunted desk lamp: switches on at night, flickers, slowly looks around
//   * beach ball: rolls around by itself at night (or is pushed by the user)
//   * ghost: fades in at night and floats around the room
class Environment {
public:
	void Init(const RoomRig& rig);
	void Update(float dt, float time, bool lampSelected, bool ballSelected, bool ghostSelected);

	// ---- clock ----
	float hour = 20.5f;             // 0..24
	float dayLengthSeconds = 150.0f; // real seconds for 24 in-game hours
	float timeScale = 1.0f;
	bool paused = false;
	void Scrub(float hours);

	float Daylight() const { return daylight; }    // 0 night .. 1 day
	float Night() const { return 1.0f - daylight; }
	bool IsNight() const { return daylight < 0.5f; }
	std::string ClockText() const;

	glm::vec3 AmbientLight() const;
	// The sun as seen from the garden: the same orbit angle as the sun behind the window, on a large
	// arc just in front of the sky backdrop. SunHeight() = sin(orbit angle), > 0 while the sun is up.
	glm::vec3 OutdoorSunPosition() const;
	float SunHeight() const;
	glm::vec3 ClearColor() const;

	// Direction the sky light travels, its colour and intensity (sun by day, moon by night).
	glm::vec3 skyLightDirection{ 0.0f, -1.0f, 0.0f };
	glm::vec3 skyLightColor{ 1.0f };
	float skyLightIntensity = 1.0f;

	// ---- lamp ----
	bool lampPower = true;
	bool lampManual = false;   // user switched it: stop following the day/night automation
	float lampIntensity = 1.0f; // after flicker
	float lampBrightness = 1.6f; // user-adjustable base intensity
	void ToggleLamp() { lampPower = !lampPower; lampManual = true; }
	void DriveLamp(float swivel, float tilt, float dt); // manual lamp control (A/D, W/S)

	// ---- ball ----
	void PushBall(const glm::vec3& direction, float dt);
	void StopBall() { ballVelocity = glm::vec3(0.0f); }

	// ---- ghost ----
	float ghostVisibility = 0.0f;
	bool hauntingEnabled = true; // story on/off also controls automatic props

	const RoomRig& Rig() const { return rig; }

private:
	void UpdateSky();
	void UpdateLamp(float dt, float time, bool selected);
	void UpdateBall(float dt, float time, bool selected);
	void UpdateGhost(float dt, float time, bool selected);

	RoomRig rig;
	float daylight = 0.0f;
	float lampBaseYaw = 20.0f;
	float lampBaseTilt = -70.0f;
	glm::vec3 ballVelocity{ 0.0f };
	float ballAngle = 0.0f;
};
