#pragma once

#include <vector>

#include <glm/glm.hpp>

class Character;

// The "movie": at night the toys come alive and wander along their own patrol loops; when morning
// comes they walk back to where the child left them, face the original direction and freeze.
//
// The character the user has selected is never driven by the story, so manual control always wins.
class StoryDirector {
public:
	void Add(Character* character, std::vector<glm::vec3> patrol, bool flies = false);
	void Update(float dt, bool night, const Character* selected, const Character* excluded);

	bool enabled = true;

private:
	struct Actor {
		Character* character;
		std::vector<glm::vec3> patrol;
		size_t next = 0;
		bool flies = false;
	};
	std::vector<Actor> actors;
};
