#include "PennyArrival.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>

#include "Room.h"
#include "characters/Cat.h"
#include "scene/SceneNode.h"

namespace {
using namespace RoomSize;

float Smooth(float t)
{
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

// Waypoint indices that mark story beats along the route.
constexpr size_t GateIndex = 2, FrontDoorIndex = 6, StairFootIndex = 9, StairTopIndex = 10, RoomDoorIndex = 12;
}

void PennyArrival::Init(Cat* penny, const HouseRig& rig)
{
	cat = penny;
	house = rig;
	const float porch = Ground + 0.45f;
	route = {
		{2.0f, Ground, 23.4f},                 // 0 on the pavement
		{11.0f, Ground, 23.3f},                // 1 along the pavement
		{12.0f, Ground, 21.9f},                // 2 through the gate
		{12.0f, Ground, 14.0f},                // 3 up the garden path
		{12.0f, porch, 12.5f},                 // 4 up the porch steps
		{12.0f, porch, 9.9f},                  // 5 across the porch
		{12.0f, Ground + 0.02f, 8.6f},         // 6 over the threshold
		{12.0f, Ground + 0.02f, -7.9f},        // 7 along the corridor
		{15.0f, Ground + 0.02f, -8.1f},        // 8 onto the stair landing
		{15.0f, Ground + 0.02f, StairBottomZ}, // 9 foot of the stairs
		{15.0f, 0.02f, StairTopZ},             // 10 top of the stairs (slope 29.4 degrees)
		{15.0f, 0.0f, 1.6f},                   // 11 into the hallway
		{10.6f, 0.0f, 1.6f},                   // 12 through the toy room door
		{8.0f, 0.0f, 1.4f},                    // 13 past the crate
		{7.3f, 0.0f, -2.4f},                   // 14 in front of the bed
	};
	routeDistance.assign(route.size(), 0.0f);
	for (size_t i = 1; i < route.size(); ++i)
		routeDistance[i] = routeDistance[i - 1] + glm::distance(route[i - 1], route[i]);
	Restart();
}

void PennyArrival::Restart()
{
	stage = Stage::Walk;
	next = 1;
	time = stageTime = travelled = 0.0f;
	cat->Root()->local.position = route[0];
	cat->Root()->local.rotation = {0, 90, 0};
	cat->SetPose(Cat::Pose::Walk);
	cat->SetSlope(0);
	cat->StopLooking();
	exteriorVisible = true;
	hour = 16.3f;
	frontDoorAngle = roomDoorAngle = stairDoorAngle = 0.0f;
	ApplyDoors(0.0f, true);
	cameraPos = {0.0f, 9.0f, 52.0f};
	cameraTarget = {3.5f, 3.0f, 0.0f};
}

void PennyArrival::Skip()
{
	stage = Stage::Done;
	next = route.size();
	cat->Root()->local.position = BedSpot();
	cat->Root()->local.rotation = {0, -45, 0};
	cat->SetPose(Cat::Pose::Sit);
	cat->SetSlope(0);
	exteriorVisible = false;
	frontDoorAngle = 85.0f;
	roomDoorAngle = 90.0f;
	stairDoorAngle = 0.0f;
	ApplyDoors(0.0f, true);
	hour = 0.0f;
}

void PennyArrival::ApplyDoors(float dt, bool instant)
{
	auto ease = [&](float& angle, float target) {
		angle = instant ? target : angle + (target - angle) * (1.0f - std::exp(-dt * 2.5f));
	};
	if (!instant) {
		const glm::vec3 p = cat->Root()->local.position;
		ease(frontDoorAngle, glm::distance(p, glm::vec3(12.0f, Ground, 9.2f)) < 4.0f || next > FrontDoorIndex ? 85.0f : 0.0f);
		ease(roomDoorAngle, next > StairFootIndex && p.z > -2.0f ? 90.0f : 0.0f);
		if (next > RoomDoorIndex) roomDoorAngle = std::max(roomDoorAngle, 89.0f); // never swings back
		// The stair door is open while she climbs and swings shut once she is in the toy room.
		ease(stairDoorAngle, next >= StairFootIndex && next <= RoomDoorIndex + 1 && stage == Stage::Walk ? 90.0f : 0.0f);
	}
	house.frontDoor->local.rotation.y = frontDoorAngle;
	house.roomDoorLeft->local.rotation.y = roomDoorAngle;
	house.roomDoorRight->local.rotation.y = -roomDoorAngle;
	house.stairDoor->local.rotation.y = stairDoorAngle;
}

void PennyArrival::Update(float dt)
{
	if (stage == Stage::Done)
		return;
	time += dt;
	stageTime += dt;
	const float k = 1.0f - std::exp(-dt * 2.6f);
	SceneNode* root = cat->Root();

	if (stage == Stage::Walk) {
		// Pause briefly on the pavement for the establishing shot, then trot along the route.
		if (time > 1.5f && cat->FollowWaypoint(route[next], dt, 2.0f)) {
			if (++next == route.size()) { stage = Stage::Jump; stageTime = 0.0f; jumpFrom = root->local.position; }
		}
		// Distance walked = route distance to the previous waypoint + progress along the current leg.
		if (next < route.size())
			travelled = routeDistance[next - 1] + glm::distance(route[next - 1], root->local.position);
		// On the flight of stairs the body pitches nose-up by the stair angle.
		cat->SetSlope(next == StairTopIndex ? glm::degrees(std::atan2(-Ground, StairTopZ - StairBottomZ)) : 0.0f);
		if (next >= StairFootIndex) exteriorVisible = false;
	}
	else if (stage == Stage::Jump) {
		// Ballistic-looking hop: straight-line interpolation plus a sine arc, 0.7 s.
		const float t = std::min(1.0f, stageTime / 0.7f);
		glm::vec3 p = glm::mix(jumpFrom, BedSpot(), Smooth(t));
		p.y += std::sin(t * glm::pi<float>()) * 1.0f;
		root->local.position = p;
		cat->Stop();
		if (t >= 1.0f) { stage = Stage::Settle; stageTime = 0.0f; cat->SetPose(Cat::Pose::Sit); }
	}
	else if (stage == Stage::Settle) {
		// Turn to face the middle of the room, sit and look around for a moment.
		root->local.rotation.y += WrapDegrees(-45.0f - root->local.rotation.y) * k;
		cat->LookAt({0.0f, 1.0f, 1.0f});
		if (stageTime > 2.0f) stage = Stage::Done;
	}
	ApplyDoors(dt, false);

	// Sunset while she walks: 16:18 on the street, dusk at the door, night upstairs.
	const float total = routeDistance.back();
	hour = std::fmod(16.3f + 7.6f * Smooth(travelled / total) + (stage != Stage::Walk ? 0.1f : 0.0f), 24.0f);

	// Camera: establishing push-in for 5 s, then a chase camera; inside the toy room a fixed vantage.
	const glm::vec3 p = root->local.position;
	const float h = glm::radians(cat->Heading());
	const glm::vec3 forward(std::sin(h), 0.0f, std::cos(h));
	glm::vec3 desiredPos, desiredTarget;
	const bool outside = next <= FrontDoorIndex;
	if (next > RoomDoorIndex || stage != Stage::Walk) {
		desiredPos = {3.2f, 3.4f, 2.6f};
		desiredTarget = cat->HeadPosition();
	}
	else {
		const float back = outside ? 4.2f : 2.4f, up = outside ? 1.9f : 1.3f;
		desiredPos = p - forward * back + glm::vec3(0.0f, up, 0.0f);
		desiredTarget = p + forward * 1.2f + glm::vec3(0.0f, 0.6f, 0.0f);
	}
	const float establishing = Smooth((time - 3.0f) / 4.0f);           // 0 for 3 s, then blends over 4 s
	const glm::vec3 shotPos = glm::mix(glm::vec3(0.0f, 9.0f, 52.0f), glm::vec3(6.0f, 4.0f, 36.0f), Smooth(time / 6.0f));
	const glm::vec3 shotTarget(3.5f, 3.0f, 0.0f);
	cameraPos += (glm::mix(shotPos, desiredPos, establishing) - cameraPos) * (establishing < 1.0f ? 1.0f : k);
	cameraTarget += (glm::mix(shotTarget, desiredTarget, establishing) - cameraTarget) * (establishing < 1.0f ? 1.0f : k * 1.5f);
	(void)GateIndex;
}

std::string PennyArrival::Caption() const
{
	if (next <= GateIndex) return "Late afternoon. Penny the cat comes home along the quiet street.";
	if (next <= FrontDoorIndex) return "Through the garden gate and up the porch. The front door swings open for her.";
	if (next <= StairFootIndex) return "Inside, the hall leads to the stairs. The sun is setting.";
	if (next <= RoomDoorIndex) return "Up the stairs to the toy room. Its double door opens.";
	return "Penny jumps onto the bed to watch. At midnight, the toys come alive...";
}
