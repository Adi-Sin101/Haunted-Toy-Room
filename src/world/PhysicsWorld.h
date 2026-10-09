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
	// A character's collision proxy. Long bodies (Penny, Bullseye) use a SPINE: several square boxes
	// at local z offsets along the body. One long box would cover them too, but its axis-aligned bounds
	// swell by up to 41% at 45 degrees, so the animal could no longer turn in a corridor; small square
	// boxes stay compact at any heading and still cover nose to tail.
	struct Actor { SceneNode* node; glm::vec3 half, offset; bool enabled = true, grounded=true; std::vector<float> spine; };
	struct Body {
		SceneNode* node;
		Transform initial;
		glm::vec3 velocity{0.0f}, angular{0.0f};
	};
	void Init(SceneNode& scene, const std::vector<SceneNode*>& blocks);
	void AddActor(SceneNode* node, glm::vec3 half, glm::vec3 offset);
	void EnableActor(SceneNode* node, bool enabled);
	void SetActorGrounded(SceneNode* node,bool grounded);
	void SetActorShape(SceneNode* node, glm::vec3 half, glm::vec3 offset);
	void SetActorSpine(SceneNode* node, std::vector<float> spine);
	void EnableHouse(bool on) { houseSpace=on; }
	void SetExteriorAccess(bool on) { exteriorAccess=on; }
	static float FloorHeight(const glm::vec3& feet);
	void EnableHallway(bool on) { hallway = on; }
	// Live takeover retains scenery contacts without blocking another actor's scripted route.
	void SetIndependentActor(SceneNode* node) { independentActor = node; }
    void SetScriptedActors(bool on) { scriptedActors=on; }
	void Update(float dt);
	void ResetBlocks();
	void PushBlock(SceneNode* node, const glm::vec3& impulse, const glm::vec3& angular=glm::vec3(0));
	void StopBlock(SceneNode* node);
	bool IsBlock(SceneNode* node) const;
	void ConstrainActor(SceneNode* node, const glm::vec3& previous);
	// Contact between characters. `byPriority` lists the bodies from highest to lowest right of way.
	// Of two overlapping bodies the lower one yields the whole push (sliding against the house); only
	// if a wall pins it does the higher one give way for the rest. The `anchor` (the character the
	// player drives) never moves. Strict priorities mean one body always wins, so a narrow doorway can
	// never jam the way equal pushes would. Story followers walk with hard actor contacts off for the
	// same reason; this pass keeps them from walking through each other or through Penny anyway.
	void SeparateActors(const std::vector<SceneNode*>& byPriority, SceneNode* anchor);
	glm::vec3 MoveCamera(const glm::vec3& previous, const glm::vec3& desired) const;
	glm::vec3 CameraSightline(const glm::vec3& target, const glm::vec3& desired, SceneNode* ignored) const;
	float FireLaser(const glm::vec3& origin, const glm::vec3& direction, float dt, SceneNode* shooter);
	static Bounds ShapeBounds(SceneNode* node);
	const std::vector<Body>& Bodies() const { return bodies; }
	SceneNode* LastLaserHit() const { return lastLaserHit; }
private:
	glm::vec3 Move(const glm::vec3& from, const glm::vec3& to, const glm::vec3& half, SceneNode* ignore, bool withActors = true) const;
	std::vector<Bounds> Obstacles(SceneNode* ignore, bool actorContacts=true) const;
	void AppendActorBounds(const Actor& actor, std::vector<Bounds>& out) const; // one box per spine segment
	glm::vec3 SlideActor(const Actor& actor, const glm::vec3& from, const glm::vec3& to, bool withActors) const;
	void RefreshScenery();
	std::vector<SceneNode*> scenery;
	std::vector<Bounds> sceneryBounds; // world boxes of `scenery`, recomputed once per Update
	std::vector<std::vector<const Bounds*>> nearby; // broad phase: scenery close to each block, per step
	std::vector<Actor> actors;
	std::vector<Body> bodies;
	float accumulator = 0.0f, laserCooldown = 0.0f;
	bool hallway = false, houseSpace=false, exteriorAccess=false, scriptedActors=false;
	SceneNode* lastLaserHit = nullptr;
	SceneNode* independentActor = nullptr;
};
