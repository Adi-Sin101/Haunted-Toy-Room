#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "House.h"

class Cat;

// Night arrival through the pavement, garden, porch and existing stairs.
// Twelve waypoints finish at the upper hallway before the locked puzzle door.
// Hinges and the chase camera use exponential smoothing; the route drives 20:30-23:00.
class PennyArrival {
public:
	void Init(Cat* cat, const HouseRig& house);
	void Restart();                 // back to the street, doors closed, outside visible
	void Skip();                    // finish immediately: Penny at the upper hallway
	void Update(float dt);

	bool Active() const { return stage != Stage::Done; }
	bool ExteriorVisible() const { return exteriorVisible; }
	bool InteriorVisible() const { return stage != Stage::Done; }
	float Hour() const { return hour; }
	const glm::vec3& CameraPosition() const { return cameraPos; }
	const glm::vec3& CameraTarget() const { return cameraTarget; }
	std::string Caption() const;

private:
	enum class Stage { Walk, Done };
	void ApplyDoors(float dt, bool instant);

	Cat* cat = nullptr;
	HouseRig house;
	std::vector<glm::vec3> route;
	std::vector<float> routeDistance;   // distance along the route at each waypoint
	size_t next = 0;
	Stage stage = Stage::Done;
	float time = 0.0f, stageTime = 0.0f, travelled = 0.0f;
	float frontDoorAngle = 0.0f, roomDoorAngle = 0.0f, stairDoorAngle = 0.0f;
	glm::vec3 cameraPos{0.0f}, cameraTarget{0.0f};
	float hour = 20.5f;
	bool exteriorVisible = true;
};
