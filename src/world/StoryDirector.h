#pragma once

#include <functional>
#include <string>

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
	void BeginGameplay();
	bool Advance(Transition transition);
	void Pause();
	void SetControlled(Character* actor, Character* passenger=nullptr) { controlled=actor; attached=passenger; }
	bool Controls(const Character* actor) const { return actor && (actor==controlled || actor==attached); }

	std::string Title() const;
	std::string Caption() const;
	std::string Objective() const;
	GameplayState CurrentState() const { return state; }
	bool PuzzleAvailable() const { return state == GameplayState::PUZZLE; }
	bool EntranceDoorLocked() const { return state != GameplayState::PROLOGUE; }
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
	Character* controlled=nullptr;
	Character* attached=nullptr;
	GameplayState state = GameplayState::PROLOGUE;
};
