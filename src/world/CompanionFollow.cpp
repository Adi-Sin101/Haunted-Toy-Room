#include "CompanionFollow.h"

#include <algorithm>
#include <cmath>

namespace {
float Wrap(float deg)
{
	deg = std::fmod(deg + 180.0f, 360.0f);
	return (deg < 0.0f ? deg + 360.0f : deg) - 180.0f;
}
}

// =============================================================================================
// FormationAnchor
// =============================================================================================
void FormationAnchor::Reset(const glm::vec3& leader, float headingDeg)
{
	position = lastLeader = leader;
	heading = Wrap(headingDeg);
	trail.assign(1, leader);
	idle = 0.0f;
	valid = true; following = false;
}

void FormationAnchor::Update(const glm::vec3& leader, float dt, const FollowSettings& s)
{
	const glm::vec2 step(leader.x - lastLeader.x, leader.z - lastLeader.z);
	lastLeader = leader;
	if (!following) {
		// Standing, turning on the spot and short steps leave the formation (and so every slot) alone.
		if (glm::length(glm::vec2(leader.x - position.x, leader.z - position.z)) <= s.followStartDistance) return;
		following = true; idle = 0.0f;
	}
	position = leader;
	const glm::vec3& last = trail.back();
	if (glm::length(glm::vec2(leader.x - last.x, leader.z - last.z)) >= s.trailSpacing) {
		trail.push_back(leader);
		// Forget the far end of the path once it is longer than any slot needs.
		float length = 0.0f; size_t keep = trail.size() - 1;
		while (keep > 0 && length < s.trailLength) { length += glm::distance(trail[keep], trail[keep - 1]); --keep; }
		trail.erase(trail.begin(), trail.begin() + static_cast<std::ptrdiff_t>(keep));
	}
	const float moved = glm::length(step);
	if (dt > 0.0f && moved > s.leaderIdleSpeed * dt) {
		idle = 0.0f;
		// Behind = against her direction of travel, so backing up (S) does not put the slots in her way.
		const float travel = glm::degrees(std::atan2(step.x, step.y));
		const float turn = s.formationTurnRate * dt;
		heading = Wrap(heading + std::clamp(Wrap(travel - heading), -turn, turn));
	} else if ((idle += dt) >= s.leaderIdleTime) {
		following = false; // freeze here: the companions settle into these slots and stay
	}
}

glm::vec3 FormationAnchor::Slot(float side, float behind, glm::vec3* alongPath) const
{
	// Character convention: heading 0 faces +Z, forward = (sin h, 0, cos h). Before the trail has a
	// segment, the path runs straight back from the anchor against its heading.
	const float h = glm::radians(heading);
	glm::vec3 forward(std::sin(h), 0.0f, std::cos(h));
	glm::vec3 point = position;
	float remaining = behind;
	for (size_t i = trail.size(); i-- > 0;) {
		const glm::vec3 older = trail[i];
		const glm::vec3 segment = point - older;
		const float length = glm::length(glm::vec2(segment.x, segment.z));
		if (length < 1e-4f) continue;
		forward = glm::vec3(segment.x, 0.0f, segment.z) / length;
		if (length >= remaining) { point -= segment * (remaining / length); remaining = 0.0f; break; }
		remaining -= length; point = older;
	}
	point -= forward * remaining;
	if (alongPath) *alongPath = point;
	const glm::vec3 right(-forward.z, 0.0f, forward.x); // her right: (-cos h, 0, sin h) for forward (sin h, 0, cos h)
	return point + right * side;
}

// =============================================================================================
// CompanionFollower
// =============================================================================================
void CompanionFollower::Rejoin(float slotDistance, const FollowSettings& s)
{
	if (slotDistance > s.followStartDistance) { mode = Mode::Approaching; best = slotDistance; stall = 0.0f; }
	else Settle();
}

CompanionFollower::Command CompanionFollower::Update(float slotDistance, float leaderDistance, bool groupFollowing, bool direct, float dt, const FollowSettings& s)
{
	if (mode == Mode::Settled) {
		if (!groupFollowing || slotDistance <= s.resumeDistance) return {};
		mode = Mode::Approaching; best = slotDistance; stall = 0.0f;
	}
	if (!groupFollowing) {
		// The leader stands: finish the approach and stop for good.
		if (slotDistance <= s.arrivalTolerance || leaderDistance < s.stopDistance) { Settle(); return {}; }
		if (!direct || slotDistance > s.catchUpDistance || slotDistance < best - s.stallProgress) { best = slotDistance; stall = 0.0f; }
		else if ((stall += dt) > s.stallTime) { Settle(); return {}; } // blocked short of the slot: stay put
	} else {
		best = slotDistance; stall = 0.0f;
	}
	// Walking pace near the slot, rising smoothly to catchUpSpeed at catchUpDistance and beyond.
	const float cap = s.walkSpeed + (s.catchUpSpeed - s.walkSpeed) * std::min(1.0f, slotDistance / s.catchUpDistance);
	const float brake = std::sqrt(2.0f * s.braking * std::max(0.0f, slotDistance - s.arrivalTolerance));
	const float desired = std::clamp(brake, s.minimumSpeed, cap);
	const float step = s.acceleration * dt;
	speed += std::clamp(desired - speed, -step, step);
	// Keeping pace inside the slot while the leader walks: no command, so nobody twitches on the spot.
	if (slotDistance <= s.arrivalTolerance) { speed = std::min(speed, s.minimumSpeed); return {}; }
	return {true, speed};
}
