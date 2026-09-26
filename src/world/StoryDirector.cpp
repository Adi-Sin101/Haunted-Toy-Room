#include "StoryDirector.h"

#include <algorithm>

#include "characters/Character.h"
#include "scene/SceneNode.h"

void StoryDirector::Add(Character* character, std::vector<glm::vec3> patrol, bool flies)
{
	actors.push_back({ character, std::move(patrol), 0, flies });
}

void StoryDirector::Update(float dt, bool night, const Character* selected, const Character* excluded)
{
	if (!enabled)
		return;

	for (Actor& actor : actors) {
		Character* c = actor.character;
		if (c == selected || c == excluded || c->InTransition())
			continue;

		if (night && !actor.patrol.empty()) {
			// Night: follow the patrol loop, one waypoint after another.
			if (c->SteerTowards(actor.patrol[actor.next], dt))
				actor.next = (actor.next + 1) % actor.patrol.size();
		}
		else if (!c->SteerTowards(c->HomePosition(), dt, 0.8f)) {
			// Morning: walk home...
		}
		else {
			// ...then turn to the original heading and stand still.
			c->TurnTowardsHeading(c->HomeHeading(), dt);
		}

		if (actor.flies) {
			// Flyers climb to cruising height at night and land in the morning.
			const float targetAltitude = night ? 1.6f : 0.0f;
			const float y = c->Root()->local.position.y;
			const float climb = std::clamp((targetAltitude - y) * 2.0f, -1.0f, 1.0f);
			c->Root()->local.position.y = std::max(0.0f, y + climb * 1.5f * dt);
		}
	}
}
