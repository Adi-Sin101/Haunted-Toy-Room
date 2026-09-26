#include "Character.h"

#include <algorithm>
#include <cmath>

#include "scene/SceneNode.h"
#include "world/Room.h"

float WrapDegrees(float deg)
{
	deg = std::fmod(deg + 180.0f, 360.0f);
	if (deg < 0.0f)
		deg += 360.0f;
	return deg - 180.0f;
}

Character::Character(std::string characterName, SceneNode* rootNode)
	: name(std::move(characterName)), root(rootNode) {}

float Character::Heading() const
{
	return root->local.rotation.y;
}

glm::vec3 Character::Forward() const
{
	const float h = glm::radians(Heading());
	return { std::sin(h), 0.0f, std::cos(h) };
}

bool Character::IsMoving() const
{
	return std::abs(speed) > 0.05f;
}

void Character::Drive(const ControlInput& in, float dt)
{
	if (transition.active)
		return;
	OnDrive(in);

	// Speed eases toward the requested speed (constant acceleration).
	const float target = in.forward * maxSpeed * (in.run ? runMultiplier : 1.0f);
	const float step = acceleration * dt;
	speed += std::clamp(target - speed, -step, step);

	// Turning: A = +1 turns left (counter-clockwise seen from above = increasing heading).
	root->local.rotation.y = WrapDegrees(root->local.rotation.y + in.turn * turnRate * TurnFactor() * dt);

	root->local.position += Forward() * speed * dt;

	if (canFly) {
		root->local.position.y = std::clamp(root->local.position.y + in.vertical * 2.0f * dt, 0.0f, RoomSize::Height - 2.2f);
	}

	if (clampToRoom) {
		const float margin = 0.6f;
		root->local.position.x = std::clamp(root->local.position.x, -RoomSize::HalfWidth + margin, RoomSize::HalfWidth - margin);
		root->local.position.z = std::clamp(root->local.position.z, -RoomSize::HalfDepth + margin, RoomSize::HalfDepth - margin);
	}
}

void Character::Stop()
{
	speed = 0.0f;
}

std::string Character::ControlsHelp() const
{
	std::string help = "W/S move  A/D turn  Shift run  SPACE stop";
	if (canFly)
		help += "  Q/E up/down";
	if (const char* s = SpecialName())
		help += std::string("  L ") + s;
	return help;
}

void Character::Animate(float dt, float /*time*/)
{
	UpdateTransition(dt);

	// Walk phase advances with distance, so feet do not slide when speed changes.
	walkPhase += speed * dt * 4.0f;
	const float targetBlend = IsMoving() ? 1.0f : 0.0f;
	moveBlend += (targetBlend - moveBlend) * std::min(1.0f, dt * 8.0f);
}

bool Character::SteerTowards(const glm::vec3& target, float dt, float speedFactor)
{
	glm::vec3 to = target - root->local.position;
	to.y = 0.0f;
	const float distance = glm::length(to);
	if (distance < 0.35f) {
		Drive({}, dt);
		return true;
	}
	const float desired = glm::degrees(std::atan2(to.x, to.z));
	const float diff = WrapDegrees(desired - Heading());
	ControlInput in;
	in.turn = std::clamp(diff / 25.0f, -1.0f, 1.0f);
	in.forward = (std::abs(diff) < 70.0f ? 1.0f : 0.35f) * speedFactor * std::min(1.0f, distance);
	Drive(in, dt);
	return false;
}

bool Character::TurnTowardsHeading(float targetHeadingDeg, float dt)
{
	const float diff = WrapDegrees(targetHeadingDeg - Heading());
	if (std::abs(diff) < 2.0f) {
		Drive({}, dt);
		return true;
	}
	root->local.rotation.y = WrapDegrees(Heading() + std::clamp(diff, -turnRate * dt, turnRate * dt));
	Drive({}, dt);
	return false;
}

void Character::SaveHome()
{
	homePosition = root->local.position;
	homeHeading = root->local.rotation.y;
}

void Character::StartTransition(const glm::vec3& toPos, float toYaw, float duration)
{
	transition.active = true;
	transition.fromPos = root->local.position;
	transition.fromYaw = root->local.rotation.y;
	transition.toPos = toPos;
	transition.toYaw = transition.fromYaw + WrapDegrees(toYaw - transition.fromYaw); // shortest way round
	transition.t = 0.0f;
	transition.duration = std::max(duration, 0.01f);
	speed = 0.0f;
}

void Character::UpdateTransition(float dt)
{
	if (!transition.active)
		return;
	transition.t = std::min(1.0f, transition.t + dt / transition.duration);
	// Smoothstep easing: starts and ends gently.
	const float s = transition.t * transition.t * (3.0f - 2.0f * transition.t);
	glm::vec3 p = glm::mix(transition.fromPos, transition.toPos, s);
	p.y += std::sin(transition.t * 3.14159265f) * 0.4f; // small hop arc
	root->local.position = p;
	root->local.rotation.y = glm::mix(transition.fromYaw, transition.toYaw, s);
	if (transition.t >= 1.0f) {
		transition.active = false;
		root->local.position = transition.toPos;
		root->local.rotation.y = WrapDegrees(transition.toYaw);
	}
}
