#pragma once

#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "House.h"
#include "StoryProps.h"

class Assets;
class SceneNode;
class Cat;
class Character;
class Humanoid;
class Bullseye;
class Buzz;
class RCCar;
class PhysicsWorld;

// The story layer: one state machine for "The Midnight Mission".
//
//   PROLOGUE      night outside; Penny explores the garden and walks in, the front door locks behind her
//   PUZZLE        upstairs hallway: train 2, clock 5, blocks 7 -> code 257 opens the Toy Room
//   TOY_CHEST     Penny presses the red wind-up button; Woody, Jessie and Bullseye climb out alive
//   BUZZ_ROOM     everyone goes downstairs; Penny opens the ordinary door on the corridor's left
//   WARDROBE      the knobs are too high: Jessie rides Bullseye, he jumps, she opens it; Buzz flies out
//   FINAL_ESCAPE  the main door is locked; Buzz flies into position and the player fires his laser
//   MORNING       everyone walks or flies outside while night turns into morning; the camera pulls back
//   FREE_EXPLORE  the story stays open: explore the neighbourhood with any character
//
// The player controls one character at a time (the player layer). Every character the player is NOT
// controlling performs its story behaviour here: the freed toys follow the leader through a small
// graph of doorway nodes, Jessie and Bullseye perform the wardrobe rescue, Buzz positions and aims.
class StoryDirector {
public:
	enum class GameplayState { PROLOGUE, PUZZLE, TOY_CHEST, BUZZ_ROOM, WARDROBE, FINAL_ESCAPE, MORNING, FREE_EXPLORE };
	enum class Transition { ENTERED_HOUSE, PUZZLE_SOLVED, TOYS_ALIVE, BUZZ_ROOM_OPENED, BUZZ_FREED, LEFT_HOUSE, MORNING_COMPLETE };

	struct Hooks {
		std::function<void()> mount, dismount; // the app's parent/child re-linking of Jessie and Bullseye
		std::function<bool()> mounted;
	};

	void Init(Cat* penny, Humanoid* woody, Humanoid* jessie, Bullseye* horse, Buzz* buzz, RCCar* car, PhysicsWorld* physics, Hooks hooks);
	void Build(SceneNode& world, Assets& assets, const HouseRig& house, const StoryRig& props);
	// story = true: the full story from the street at night (toys hidden in the chest and wardrobe).
	// story = false: the sandbox used by manual mode (every toy out in the Toy Room, every door open).
	void Restart(bool story);
	void Update(float dt);
	std::string Interact(Character* actor);
	bool RequestLaser();          // L while Buzz is in position at the locked door
	void Pause();
	bool Advance(Transition transition);
	void SetControlled(Character* actor, Character* passenger = nullptr);
	bool Controls(const Character* actor) const { return actor && (actor == controlled || actor == attached); }

	std::string Title() const;
	std::string Caption() const;
	std::string Objective() const;
	GameplayState CurrentState() const { return state; }
	bool Sandbox() const { return sandbox; }
	bool PuzzleAvailable() const { return state == GameplayState::PUZZLE; }
	bool Available(const Character* actor) const;   // freed (selectable) in the story
	bool SelectionOpen() const { return sandbox || toysFreed; }
	bool Hidden(const Character* actor) const;      // still inside the chest or the wardrobe
	bool Scripted(const Character* actor) const;    // in a scripted climb or flight: no physics this frame
	// On a story task right now (Jessie and Bullseye during the wardrobe rescue, Buzz flying to the door):
	// in a crowd these characters have right of way over the followers (see PhysicsWorld::SeparateActors).
	bool Busy(const Character* actor) const;
	bool FollowersActive() const;
	bool DoorBroken() const { return doorBroken; }
	bool BuzzReady() const { return buzzReady && !doorBroken; }
	bool ExteriorVisible() const { return state == GameplayState::PROLOGUE || doorBroken; }
	bool Escaped() const { return state == GameplayState::MORNING || state == GameplayState::FREE_EXPLORE; }
	float Hour() const { return hour; }
	// Camera shot the story asks for (the morning pull-back); false = the player's camera.
	bool Cinematic(glm::vec3& position, glm::vec3& target) const;
	// A pulsing story light: the chest button's red glow, then the green glow inside the wardrobe.
	bool Glow(glm::vec3& position, glm::vec3& color, float& intensity) const;
	const std::vector<SceneNode*>& Debris() const { return debris; }
	bool enabled = true;

private:
	struct Nav { int node = -1, goal = -1; bool moving = false; };
	struct Emergence { Character* actor; float delay; glm::vec3 rise, land; float heading; int phase = 0; };
	enum class Rescue { Waiting, Mounting, Riding, Aligning, Jumping, StepAside, BuzzFlight, Done };

	void Enter(GameplayState next);
	void AnimateProps(float dt);
	void UpdateChest(float dt);
	void UpdateRescue(float dt);
	void UpdateEntrance(float dt);
	void UpdateFollowers(float dt, const std::vector<Character*>& exclude);
	void Gather(float dt, bool withPenny = false); // everyone to their garden spot (Penny too, unless the player drives her)
	void PressChest();
	void OpenWardrobe();
	void TryDoor();
	void BreakDoor();
	bool Mounted() const { return hooks.mounted && hooks.mounted(); }
	Character* Leader() const;

	// Navigation: zones (Toy Room, upper hall, stairs, ground floor, Buzz's room, outside) linked by
	// doorway nodes; Floyd-Warshall gives the next node toward any goal.
	static int Zone(const glm::vec3& p);
	int NearestNode(const glm::vec3& p, int zone) const;
	bool Navigate(Character* actor, const glm::vec3& target, float dt, float speed, float arrive = 0.25f);
	bool MoveTo(Character* actor, const glm::vec3& target, float dt, float speed, float arrive);
	glm::vec3 Steer(const Character* actor, const glm::vec3& waypoint) const; // walk around bodies in the way

	Cat* penny = nullptr;
	Humanoid* woody = nullptr;
	Humanoid* jessie = nullptr;
	Bullseye* horse = nullptr;
	Buzz* buzz = nullptr;
	RCCar* car = nullptr;
	PhysicsWorld* physics = nullptr;
	Hooks hooks;
	HouseRig house;
	StoryRig props;
	SceneNode* doorLeaf = nullptr;
	SceneNode* lock = nullptr;
	std::vector<SceneNode*> debris;

	std::vector<glm::vec3> nodes;
	std::vector<int> nodeZone;
	std::vector<std::vector<int>> nextHop;
	std::vector<std::vector<float>> pathCost;
	std::map<const Character*, Nav> nav;
	// Wait spot (index into WaitSpots) of each character at the locked door, assigned by position when
	// the door is found locked: whoever is nearest the door gets the spot nearest the door, so nobody has
	// to squeeze past anyone else in the corridor.
	std::map<const Character*, int> waitSlot;

	GameplayState state = GameplayState::PROLOGUE;
	bool sandbox = false;
	float elapsed = 0.0f, hour = 21.0f, morningStart = 23.5f;
	// Toy chest
	bool chestPressed = false, toysFreed = false;
	float chestOpen = 0.0f, chestTime = 0.0f, buttonPress = 0.0f;
	std::vector<Emergence> emergences;
	// Buzz's room and the wardrobe
	bool buzzDoorOpen = false, rescueRequested = false, wardrobeOpening = false, buzzFreed = false;
	float buzzDoorAngle = 0.0f, wardrobeOpen = 0.0f, rescueTime = 0.0f, rattle = 0.0f;
	Rescue rescue = Rescue::Waiting;
	size_t buzzLeg = 0;
	// Entrance
	bool doorTried = false, buzzReady = false, doorBroken = false;
	float frontDoorAngle = 0.0f, doorShake = 0.0f, tryTimer = 0.0f, firing = 0.0f;
	// Morning
	float morningTime = 0.0f, pullback = 0.0f;

	Character* controlled = nullptr;
	Character* attached = nullptr;
};
