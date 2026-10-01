#include "Environment.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <glm/gtc/constants.hpp>

#include "math/Transform3D.h"
#include "render/ProceduralTextures.h"
#include "scene/Material.h"
#include "scene/SceneNode.h"

namespace {
float Smooth(float e0, float e1, float x)
{
	const float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}
}

void Environment::Init(const RoomRig& roomRig)
{
	rig = roomRig;
	UpdateSky();
}

void Environment::Scrub(float hours)
{
	hour = std::fmod(hour + hours + 24.0f, 24.0f);
}

std::string Environment::ClockText() const
{
	const int h = static_cast<int>(hour);
	const int m = static_cast<int>((hour - static_cast<float>(h)) * 60.0f);
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%02d:%02d %s", h, m, IsNight() ? "Night" : "Day");
	return buf;
}

glm::vec3 Environment::AmbientLight() const
{
	return glm::mix(glm::vec3(0.18f, 0.19f, 0.26f), glm::vec3(0.48f, 0.46f, 0.43f), daylight);
}

float Environment::SunHeight() const
{
	return std::sin((hour - 6.0f) / 12.0f * glm::pi<float>());
}

glm::vec3 Environment::OutdoorSunPosition() const
{
	const float a = (hour - 6.0f) / 12.0f * glm::pi<float>();
	return {-std::cos(a) * 60.0f, 4.0f + std::sin(a) * 55.0f, -37.0f};
}

glm::vec3 Environment::ClearColor() const
{
	return glm::mix(glm::vec3(0.02f, 0.02f, 0.05f), glm::vec3(0.35f, 0.45f, 0.6f), daylight);
}

void Environment::Update(float dt, float time, bool lampSelected, bool ballSelected, bool ghostSelected)
{
	if (!paused)
		Scrub(dt * timeScale * 24.0f / dayLengthSeconds);
	UpdateSky();
	UpdateLamp(dt, time, lampSelected);
	UpdateBall(dt, time, ballSelected);
	UpdateGhost(dt, time, ghostSelected);
}

// The sun follows a half ellipse behind the window: angle a = 0 at 06:00 (rising), PI at 18:00.
// The moon is on the opposite side of the same ellipse.
void Environment::UpdateSky()
{
	const float a = (hour - 6.0f) / 12.0f * glm::pi<float>();
	const float sunHeight = std::sin(a);
	auto orbit = [&](float angle) {
		return rig.skyCenter + glm::vec3(-std::cos(angle) * 3.2f, std::sin(angle) * 3.1f, 0.0f);
	};
	const glm::vec3 sunPos = orbit(a);
	const glm::vec3 moonPos = orbit(a + glm::pi<float>());
	rig.sun->local.position = sunPos;
	rig.moon->local.position = moonPos;
	rig.sun->visible = sunHeight > -0.2f;
	rig.moon->visible = sunHeight < 0.2f;

	daylight = Smooth(-0.1f, 0.25f, sunHeight);
	const glm::vec3 roomCenter(0.0f, 1.0f, 0.0f);
	if (sunHeight >= 0.0f) {
		skyLightDirection = glm::normalize(roomCenter - sunPos);
		const float sunset = 1.0f - Smooth(0.0f, 0.4f, sunHeight);
		skyLightColor = glm::mix(glm::vec3(1.0f, 0.95f, 0.85f), glm::vec3(1.0f, 0.55f, 0.3f), sunset);
		skyLightIntensity = 0.9f * Smooth(-0.05f, 0.3f, sunHeight);
	}
	else {
		skyLightDirection = glm::normalize(roomCenter - moonPos);
		skyLightColor = glm::vec3(0.55f, 0.65f, 1.0f);
		skyLightIntensity = 0.8f * Smooth(-0.05f, 0.3f, -sunHeight);
	}

	// Sky backdrop: emissive = sky colour, colour = star brightness (stars fade out by day).
	const float sunset = std::exp(-std::abs(sunHeight) * 6.0f);
	glm::vec3 skyColor = glm::mix(glm::vec3(0.02f, 0.03f, 0.10f), glm::vec3(0.45f, 0.65f, 0.95f), daylight);
	skyColor += glm::vec3(0.55f, 0.22f, 0.05f) * sunset;
	rig.skyMaterial->emissive = skyColor;
	rig.skyMaterial->color = glm::vec3(1.0f - daylight);
	// Neighbouring houses: dark silhouettes at night, muted blue-grey by day.
	if (rig.skylineMaterial)
		rig.skylineMaterial->color = glm::mix(glm::vec3(0.035f, 0.045f, 0.085f), glm::vec3(0.50f, 0.52f, 0.60f), daylight);
}

void Environment::DriveLamp(float swivel, float tilt, float dt)
{
	lampBaseYaw += swivel * 90.0f * dt;
	lampBaseTilt = std::clamp(lampBaseTilt - tilt * 60.0f * dt, -130.0f, 20.0f);
}

void Environment::UpdateLamp(float /*dt*/, float time, bool selected)
{
	if (!lampManual)
		lampPower = IsNight();

	// Haunting: when the lamp is not being controlled it slowly looks around at night.
	const float haunt = (hauntingEnabled && !selected) ? Night() : 0.0f;
	rig.lamp->local.rotation.y = lampBaseYaw + std::sin(time * 0.35f) * 35.0f * haunt;
	rig.lampHead->local.rotation.x = lampBaseTilt + std::sin(time * 0.8f) * 12.0f * haunt;

	// Flicker = layered noise, plus rare short drop-outs.
	float flicker = 1.0f;
	if (haunt > 0.0f) {
		const float n = ProceduralTextures::ValueNoise(time * 9.0f, 3.1f);
		flicker = 0.8f + 0.2f * n;
		if (ProceduralTextures::ValueNoise(time * 1.7f, 7.7f) > 0.78f)
			flicker *= 0.15f;
		flicker = glm::mix(1.0f, flicker, haunt);
	}
	lampIntensity = lampPower ? lampBrightness * flicker : 0.0f;
	rig.bulbMaterial->emissive = glm::vec3(1.0f, 0.9f, 0.6f) * (lampPower ? 0.2f + 0.8f * flicker : 0.05f);
}

void Environment::PushBall(const glm::vec3& direction, float dt)
{
	ballVelocity += glm::vec3(direction.x, 0.0f, direction.z) * 6.0f * dt;
}

void Environment::UpdateBall(float dt, float time, bool selected)
{
	SceneNode* ball = rig.ball;
	const float r = rig.ballRadius;
	glm::vec3 previous = ball->local.position;

	if (!selected && hauntingEnabled && IsNight()) {
		// Autonomous: a slow looping path (Lissajous figure) across the free floor space.
		ballAngle += dt * 0.35f;
		const glm::vec3 target(-2.0f + 3.0f * std::sin(ballAngle), r, 2.5f + 1.8f * std::sin(ballAngle * 2.0f));
		ballVelocity = glm::mix(ballVelocity, (target - previous) * 1.5f, std::min(1.0f, dt * 2.0f));
	}
	else {
		ballVelocity *= std::max(0.0f, 1.0f - dt * 0.8f); // rolling friction
	}
	(void)time;

	glm::vec3 p = previous + ballVelocity * dt;
	// Bounce off the walls: reflect the velocity component normal to the wall.
	const float limX = RoomSize::HalfWidth - r, limZ = RoomSize::HalfDepth - r;
	if (p.x < -limX || p.x > limX) { ballVelocity.x = -ballVelocity.x * 0.8f; p.x = std::clamp(p.x, -limX, limX); }
	if (p.z < -limZ || p.z > limZ) { ballVelocity.z = -ballVelocity.z * 0.8f; p.z = std::clamp(p.z, -limZ, limZ); }
	p.y = r;
	ball->local.position = p;

	// Rolling without slipping: rotate about axis = up x velocity by angle = distance / radius.
	const glm::vec3 moved = p - previous;
	const float distance = glm::length(glm::vec2(moved.x, moved.z));
	if (distance > 1e-5f) {
		const glm::vec3 axis = glm::normalize(glm::cross(glm::vec3(0, 1, 0), glm::vec3(moved.x, 0.0f, moved.z)));
		rig.ballShape->local.basis = t3d::rotateAxis(axis, distance / r) * rig.ballShape->local.basis;
	}
}

void Environment::UpdateGhost(float dt, float time, bool selected)
{
	const float target = hauntingEnabled ? Night() : 0.0f;
	ghostVisibility += (target - ghostVisibility) * std::min(1.0f, dt * 1.5f);
	rig.ghost->visible = ghostVisibility > 0.02f || selected;
	rig.ghostMaterial->opacity = 0.55f * (selected ? 1.0f : ghostVisibility);

	if (selected)
		return; // hold still while being inspected / edited

	// Circles the room with a bobbing motion and faces its direction of travel.
	const float a = time * 0.25f;
	const glm::vec3 p(std::sin(a) * 4.5f, 4.2f + std::sin(time * 1.3f) * 0.35f, std::cos(a) * 3.0f - 0.5f);
	const glm::vec3 v(std::cos(a) * 4.5f, 0.0f, -std::sin(a) * 3.0f);
	rig.ghost->local.position = p;
	rig.ghost->local.rotation.y = glm::degrees(std::atan2(v.x, v.z));
	rig.ghostBody->local.rotation.z = std::sin(time * 1.1f) * 8.0f;
}
