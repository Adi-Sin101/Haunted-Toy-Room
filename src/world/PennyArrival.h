#pragma once

#include <string>

#include <glm/glm.hpp>

#include "House.h"

class Cat;

// The opening shot of the story: a slow crane from high above the street down to a chase position
// behind Penny, who sits on the pavement looking at the abandoned house. Then the player takes over
// (the story's PROLOGUE: explore the garden and walk in). Y skips it; the camera uses smoothstep.
class PennyArrival {
public:
	void Init(Cat* cat, const HouseRig& house);
	void Restart();                 // back to the start of the shot
	void Skip();                    // end the shot now
	void Update(float dt);

	bool Active() const { return active; }
	bool ExteriorVisible() const { return active; }
	bool InteriorVisible() const { return active; }
	float Hour() const { return 20.75f; }
	const glm::vec3& CameraPosition() const { return cameraPos; }
	const glm::vec3& CameraTarget() const { return cameraTarget; }
	std::string Caption() const;

	static constexpr float Duration = 7.0f;

private:
	Cat* cat = nullptr;
	HouseRig house;
	bool active = false;
	float time = 0.0f;
	glm::vec3 cameraPos{0.0f}, cameraTarget{0.0f};
};
