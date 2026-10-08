#pragma once

#include <functional>
#include <string>
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include "House.h"

class Assets;
class SceneNode;
class Cat;
class Character;
class Humanoid;
class Bullseye;
class Buzz;
class RCCar;
class PhysicsWorld;

// Owns the single progression state machine for the toy-room story.
class StoryDirector {
public:
	enum class GameplayState { PROLOGUE, PUZZLE, TOY_RESCUE, BUZZ_RESCUE, FINAL_ESCAPE, WIN };
	enum class Transition { ARRIVAL_COMPLETE, PUZZLE_SOLVED, TOYS_FREED, BUZZ_FREED, ESCAPE_COMPLETE };

	void Init(Humanoid*, Humanoid*, Bullseye*, Buzz*, RCCar*, PhysicsWorld*, std::function<void()> dismount);
	void Restart(bool mounted);
    void Build(SceneNode& world, Assets& assets, const HouseRig& rig, Cat* cat, SceneNode* ghost);
    void Update(float dt, bool mounted);
    std::string Interact(Character* actor, bool mounted);
    glm::vec3 DismountTarget(const glm::vec3& normal, bool mounted) const;
    const std::vector<SceneNode*>& Debris() const { return debris; }
    bool DoorBroken() const { return doorBroken; }
    bool ToysReleased() const { return switches[0] && switches[1] && switches[2]; }
    void BeginGameplay();
	bool Advance(Transition transition);
	void Pause();
	void SetControlled(Character* actor, Character* passenger=nullptr);
	bool Controls(const Character* actor) const { return actor && (actor==controlled || actor==attached); }

	std::string Title() const;
	std::string Caption() const;
	std::string Objective() const;
	GameplayState CurrentState() const { return state; }
	bool PuzzleAvailable() const { return state == GameplayState::PUZZLE; }
	bool EntranceDoorLocked() const { return state != GameplayState::PROLOGUE && !doorBroken; }
	bool ToyRescueAvailable() const { return state == GameplayState::TOY_RESCUE; }
	bool BuzzRescueAvailable() const { return state == GameplayState::BUZZ_RESCUE; }
	bool ChaseActive() const { return state == GameplayState::FINAL_ESCAPE; }
	bool FinalDoorSealed() const { return state == GameplayState::FINAL_ESCAPE; }
	bool Ending() const { return state == GameplayState::WIN; }
	bool ToyRoomDoorLocked() const { return state == GameplayState::PUZZLE; }
	bool enabled = true;

private:
	void Enter(GameplayState next);

	Humanoid* woody = nullptr;
	Humanoid* jessie = nullptr;
	Bullseye* horse = nullptr;
	Buzz* buzz = nullptr;
	RCCar* car = nullptr;
	PhysicsWorld* physics = nullptr;
	std::function<void()> dismount;
    HouseRig house;
    SceneNode* world=nullptr;
    SceneNode* toyBarrier=nullptr;
    SceneNode* buzzBarrier=nullptr;
    SceneNode* ghost=nullptr;
    SceneNode* doorLeaf=nullptr;
    SceneNode* lock=nullptr;
    Cat* penny=nullptr;
    std::array<bool,3> switches{};
    std::array<SceneNode*,3> switchHandles{};
    std::vector<SceneNode*> debris;
    bool ridingAtSwitch=false, doorBroken=false, finalTriggered=false;
    float elapsed=0, firing=0, chaseTime=0;
    struct EscapeRoute { Character* actor; size_t next=0; glm::vec3 progress{0}; bool done=false; };
    std::vector<EscapeRoute> escapeRoutes;
    void BeginEscape();
    void BreakDoor();
	Character* controlled=nullptr;
	Character* attached=nullptr;
	GameplayState state = GameplayState::PROLOGUE;
};
