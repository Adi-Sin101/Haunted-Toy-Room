#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "scene/Transform.h"

class SceneNode;

// Conservative transformed bounds, swept contacts and a small fixed-step block simulation.
// Physical proxies are independent of tiny decorative meshes and remain usable at high speed.
class PhysicsWorld {
public:
	struct Bounds { glm::vec3 low, high; SceneNode* node = nullptr; };
	struct Actor { SceneNode* node; glm::vec3 half, offset; bool enabled = true; };
	struct Body {
		SceneNode* node;
		Transform initial;
		glm::vec3 velocity{0.0f}, angular{0.0f};
	};
	void Init(SceneNode& scene, const std::vector<SceneNode*>& blocks);
	void AddActor(SceneNode* node, glm::vec3 half, glm::vec3 offset);
	void EnableActor(SceneNode* node, bool enabled);
	void SetActorShape(SceneNode* node, glm::vec3 half, glm::vec3 offset);
	void EnableHallway(bool on) { hallway = on; }
	void Update(float dt);
	void ResetBlocks();
	void PushBlock(SceneNode* node, const glm::vec3& impulse);
	void StopBlock(SceneNode* node);
	bool IsBlock(SceneNode* node) const;
	void ConstrainActor(SceneNode* node, const glm::vec3& previous);
	glm::vec3 MoveCamera(const glm::vec3& previous, const glm::vec3& desired) const;
	glm::vec3 CameraSightline(const glm::vec3& target, const glm::vec3& desired, SceneNode* ignored) const;
	float FireLaser(const glm::vec3& origin, const glm::vec3& direction, float dt, SceneNode* shooter);
	static Bounds ShapeBounds(SceneNode* node);
	const std::vector<Body>& Bodies() const { return bodies; }
	SceneNode* LastLaserHit() const { return lastLaserHit; }
private:
	glm::vec3 Move(const glm::vec3& from, const glm::vec3& to, const glm::vec3& half, SceneNode* ignore) const;
	std::vector<Bounds> Obstacles(SceneNode* ignore) const;
	Bounds ActorBounds(const Actor& actor) const;
	std::vector<SceneNode*> scenery;
	std::vector<Actor> actors;
	std::vector<Body> bodies;
	float accumulator = 0.0f, laserCooldown = 0.0f;
	bool hallway = false;
	SceneNode* lastLaserHit = nullptr;
};
