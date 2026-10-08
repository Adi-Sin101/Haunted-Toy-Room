#include "PennyArrival.h"

#include <algorithm>
#include <cmath>

#include "Room.h"
#include "characters/Cat.h"
#include "scene/SceneNode.h"

namespace {
float Smooth(float t)
{
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}
}

void PennyArrival::Init(Cat* penny, const HouseRig& rig)
{
	cat = penny;
	house = rig;
	Restart();
}

void PennyArrival::Restart()
{
	active = true;
	time = 0.0f;
	cat->EnableArrivalMotion(false);
	cat->SetPose(Cat::Pose::Sit);
	Update(0.0f);
}

void PennyArrival::Skip()
{
	active = false;
	cat->SetPose(Cat::Pose::Walk);
	cat->StopLooking();
}

void PennyArrival::Update(float dt)
{
	if (!active) return;
	time += dt;
	const glm::vec3 p = cat->Root()->local.position;
	const glm::vec3 forward = cat->Forward();
	// Penny looks up at the house while the camera cranes down behind her.
	cat->LookAt({12.0f, RoomSize::Ground + 5.0f, 9.0f});
	cat->SetPose(time > Duration - 1.5f ? Cat::Pose::Walk : Cat::Pose::Sit);
	const float s = Smooth(time / (Duration - 0.5f));
	const glm::vec3 high(-4.0f, 14.0f, 48.0f), chase = p - forward * 4.4f + glm::vec3(0.0f, 1.5f, 0.0f);
	const glm::vec3 facade(9.0f, RoomSize::Ground + 4.5f, 2.0f), focus = p + glm::vec3(0.0f, 0.65f, 0.0f);
	cameraPos = glm::mix(high, chase, s);
	cameraTarget = glm::mix(facade, focus, s * s);
	if (time >= Duration) Skip();
}

std::string PennyArrival::Caption() const
{
	return "Night. Penny arrives at the abandoned house. The front door stands open...";
}
