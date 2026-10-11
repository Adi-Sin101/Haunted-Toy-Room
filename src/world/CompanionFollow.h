#pragma once

#include <vector>
#include <glm/glm.hpp>

// Distance-based companion following: the freed toys accompanying Penny.
//
// Two small pieces, both plain maths so tests/PhysicsChecks.cpp can drive them without a scene:
//
//   FormationAnchor    one per group: where the formation slots are laid out from. It stays put while the
//                      leader stands still, turns on the spot or wanders less than followStartDistance. Once
//                      she goes farther it tracks her and drops a breadcrumb every trailSpacing metres, until
//                      she has stood still for leaderIdleTime; then it freezes again. A slot lies `behind`
//                      metres back along that trail (her own path, so it is walkable: stairs, corridors and
//                      doorways need no special cases) and `side` metres to the right of the path there.
//                      The slots are stable between moves and never swing round because she turned.
//
//   CompanionFollower  one per companion: Settled (no movement commands at all) or Approaching its slot.
//                      A settled companion starts again only when the group follows and its slot is more
//                      than resumeDistance away; it settles within arrivalTolerance of the slot, within
//                      stopDistance of the leader (it never presses into her), or when a wall or a crowd has
//                      kept it from getting any closer to a nearby slot in its own room for stallTime. Its
//                      speed is limited by an acceleration and by a braking curve into the slot, so it eases
//                      in and out.
struct FollowSettings {
	float followStartDistance = 2.0f;     // the leader leaves the frozen anchor by this much -> the group follows
	float preferredFollowDistance = 1.7f; // first row of formation slots behind the leader
	float stopDistance = 1.0f;            // a companion never walks closer than this to the standing leader
	float catchUpDistance = 4.5f;         // this far from its slot a companion is up to catchUpSpeed
	float arrivalTolerance = 0.3f;        // slot reached
	float resumeDistance = 1.2f;          // hysteresis: a settled companion moves again beyond this (> arrivalTolerance)
	float leaderIdleSpeed = 0.15f;        // m/s; slower than this the leader counts as standing
	float leaderIdleTime = 0.35f;         // standing this long freezes the anchor (a key tap is not a stop)
	float formationTurnRate = 150.0f;     // deg/s the group's facing (taken up on arrival) turns toward her travel
	float trailSpacing = 0.3f;            // metres between breadcrumbs of the leader's path
	float trailLength = 12.0f;            // metres of path kept (longer than the deepest slot)
	float walkSpeed = 2.1f;               // m/s next to the slot (Penny walks at 2.0, the toys at 1.8)
	float catchUpSpeed = 4.0f;            // m/s (Penny runs at 4.0)
	float acceleration = 4.0f;            // m/s^2
	float braking = 3.0f;                 // m/s^2 of the braking curve v = sqrt(2 * braking * remaining)
	float minimumSpeed = 0.35f;           // crawl speed at the end of the braking curve
	float stallTime = 1.0f;               // s without stallProgress of headway to a direct slot within catchUpDistance -> settle
	float stallProgress = 0.05f;
};

class FormationAnchor {
public:
	void Reset(const glm::vec3& leader, float headingDeg);
	void Update(const glm::vec3& leader, float dt, const FollowSettings& settings);
	bool Valid() const { return valid; }
	bool Following() const { return following; }
	const glm::vec3& Position() const { return position; }
	float Heading() const { return heading; }
	// The point `behind` metres back along the leader's path and `side` metres to the right of it there
	// (beyond the recorded trail the path continues straight back). `alongPath` receives that point
	// without the sideways offset.
	glm::vec3 Slot(float side, float behind, glm::vec3* alongPath = nullptr) const;

private:
	std::vector<glm::vec3> trail; // breadcrumbs, oldest first; the newest leads to `position`
	glm::vec3 position{0.0f}, lastLeader{0.0f};
	float heading = 0.0f, idle = 0.0f;
	bool valid = false, following = false;
};

class CompanionFollower {
public:
	enum class Mode { Settled, Approaching };
	struct Command { bool move = false; float speed = 0.0f; };

	// Back under the group's control after the player (or a story task) had this companion: it walks
	// back to its slot only if that is farther than followStartDistance, otherwise it stays where it is.
	void Rejoin(float slotDistance, const FollowSettings& settings);
	// `direct`: the slot lies in the companion's own zone, so the straight-line distance measures its
	// progress (on a route through doorways it may grow for a while without anything being wrong).
	Command Update(float slotDistance, float leaderDistance, bool groupFollowing, bool direct, float dt, const FollowSettings& settings);
	Mode CurrentMode() const { return mode; }
	float Speed() const { return speed; }

private:
	void Settle() { mode = Mode::Settled; speed = 0.0f; stall = 0.0f; }
	Mode mode = Mode::Settled;
	float speed = 0.0f, best = 0.0f, stall = 0.0f;
};
