#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "House.h"

class Cat;

// The prologue before the Midnight Mission: Penny the cat comes home in the late afternoon, walks
// along the pavement, through the garden gate, up the porch, in through the front door, along the
// ground-floor corridor, up the stairs, through the toy room's double door, and jumps onto the bed,
// from where she watches the story. The camera follows her; the sun sets while she walks.
//
//   Route      : 3D waypoints, followed at constant speed with Character::FollowWaypoint
//   Doors      : hinge angles ease open when Penny comes close (exponential approach)
//   Camera     : establishing push-in, then a smoothed chase camera behind and above her
//   Clock      : hour = 16.3 + 7.6 * smoothstep(distance walked / route length)
//   Exterior   : visible until she reaches the foot of the stairs (it cannot be seen from inside)
class PennyArrival {
public:
	void Init(Cat* cat, const HouseRig& house);
	void Restart();                 // back to the street, doors closed, outside visible
	void Skip();                    // finish immediately: Penny on the bed, doors open
	void Update(float dt);

	bool Active() const { return stage != Stage::Done; }
	bool ExteriorVisible() const { return exteriorVisible; }
	bool InteriorVisible() const { return stage != Stage::Done; } // the stair door is shut afterwards
	float Hour() const { return hour; }
	const glm::vec3& CameraPosition() const { return cameraPos; }
	const glm::vec3& CameraTarget() const { return cameraTarget; }
	std::string Caption() const;

	// Where Penny sits on the bed to watch the story.
	static glm::vec3 BedSpot() { return {7.5f, 1.17f, -4.9f}; }

private:
	enum class Stage { Walk, Jump, Settle, Done };
	void ApplyDoors(float dt, bool instant);

	Cat* cat = nullptr;
	HouseRig house;
	std::vector<glm::vec3> route;
	std::vector<float> routeDistance;   // distance along the route at each waypoint
	size_t next = 0;
	Stage stage = Stage::Done;
	float time = 0.0f, stageTime = 0.0f, travelled = 0.0f;
	float frontDoorAngle = 0.0f, roomDoorAngle = 0.0f, stairDoorAngle = 0.0f;
	glm::vec3 jumpFrom{0.0f};
	glm::vec3 cameraPos{0.0f}, cameraTarget{0.0f};
	float hour = 16.3f;
	bool exteriorVisible = true;
};
