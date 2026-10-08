#pragma once

#include <string>
#include <vector>
#include <utility>
#include "scene/Transform.h"

#include <glm/glm.hpp>

class SceneNode;

// Movement request for one frame (from the keyboard or from the autopilot).
struct ControlInput {
	float forward = 0.0f;  // -1 (S) .. +1 (W)
	float turn = 0.0f;     // -1 (D, right) .. +1 (A, left)
	float vertical = 0.0f; // -1 (E, down) .. +1 (Q, up)   - Buzz only
	bool run = false;      // Shift
};

// Base class of every controllable object (Woody, Jessie, Buzz, Bullseye, RC car).
//
// The character's state lives directly in its root node's LOCAL transform:
//   position   = root->local.position
//   heading    = root->local.rotation.y  (degrees, 0 = facing +Z)
// so the inspector (edit mode) and the controls always agree.
//
// Movement (frame-rate independent via delta time):
//   forward = (sin(heading), 0, cos(heading))
//   speed  -> approaches forward * maxSpeed with a constant acceleration
//   position += forward * speed * dt,    heading += turn * turnRate * dt
class Character {
public:
	Character(std::string name, SceneNode* root);
	virtual ~Character() = default;

	const std::string& Name() const { return name; }
	SceneNode* Root() const { return root; }

	// Manual control / autopilot share this entry point.
	void Drive(const ControlInput& in, float dt);
	void Stop();                       // SPACE: immediate stop
	virtual void Special() {}          // L key: laser, headlights...
	virtual const char* SpecialName() const { return nullptr; }
	virtual std::string ControlsHelp() const;

	// Per-frame animation (limbs, wheels). `time` is the global clock.
	virtual void Animate(float dt, float time);

	float Heading() const;
	glm::vec3 Forward() const;
	float Speed() const { return speed; }
	bool IsMoving() const;
	bool CanFly() const { return canFly; }

	// Autopilot: turn toward `target` (xz) and walk to it. Returns true when arrived.
	bool SteerTowards(const glm::vec3& target, float dt, float speedFactor = 0.6f);
	bool FollowWaypoint(const glm::vec3& target, float dt, float metresPerSecond = 1.8f);
	bool TurnTowardsHeading(float targetHeadingDeg, float dt);

	// Home = where the toy stands during the day.
	void SaveHome();
	void RestoreRestPose();
	const glm::vec3& HomePosition() const { return homePosition; }
	float HomeHeading() const { return homeHeading; }

	// Smooth move of the root's local transform (used by mount / dismount so nothing teleports).
	void StartTransition(const glm::vec3& toPos, float toYaw, float duration);
	bool InTransition() const { return transition.active; }
	void CancelTransition() { transition.active = false; }

	// Keeps the character inside the room (simple bounds, not collision detection).
	bool clampToRoom = true;

protected:
	virtual float TurnFactor() const { return 1.0f; } // car: can only turn while rolling
	virtual void OnDrive(const ControlInput& /*in*/) {}

	std::string name;
	SceneNode* root;

	float maxSpeed = 2.0f;     // units / second
	float runMultiplier = 2.0f;
	float acceleration = 8.0f; // units / second^2
	float turnRate = 120.0f;   // degrees / second
	bool canFly = false;

	float speed = 0.0f;        // signed, along Forward()
	float walkPhase = 0.0f;    // radians, advances with distance travelled
	float moveBlend = 0.0f;    // 0 = idle pose, 1 = full walk cycle (eases in/out)

private:
	void UpdateTransition(float dt);

	struct Transition {
		bool active = false;
		glm::vec3 fromPos{ 0.0f }, toPos{ 0.0f };
		float fromYaw = 0.0f, toYaw = 0.0f;
		float t = 0.0f, duration = 1.0f;
	} transition;

	glm::vec3 homePosition{ 0.0f };
	float homeHeading = 0.0f;
	std::vector<std::pair<SceneNode*, Transform>> restPose;
	Transform homeTransform;
};

// Wraps an angle difference to [-180, 180) degrees.
float WrapDegrees(float deg);
