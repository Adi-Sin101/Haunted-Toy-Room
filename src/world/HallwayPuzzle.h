#pragma once

#include <glm/glm.hpp>
#include <string>

#include "PuzzleCode.h"

class Assets;
class SceneNode;
struct HouseRig;

// Owns Stage 1's clue props, keypad session, and physical Toy Room door lock.
class HallwayPuzzle {
public:
	void Build(SceneNode& world, Assets& assets, const HouseRig& house);
	void Begin();
	void Reset();
	void Update(float dt);

	// Interact with the nearest clue or keypad using the existing Enter action.
	std::string Interact(const glm::vec3& pennyPosition);
	std::string InteractionHint(const glm::vec3& pennyPosition) const;
	bool KeypadActive() const { return keypadActive; }
	void CancelKeypad();
	bool AddDigit(int digit) { return keypadActive && code.AddDigit(digit); }
	bool Backspace() { return keypadActive && code.Backspace(); }
	PuzzleCode::Result Submit();

	const std::string& Status() const { return status; }
	std::string CodeDisplay() const { return code.Display(); }
	bool Unlocked() const { return unlocked; }
	bool CluesComplete() const { return code.AllCluesFound(); }
	int CluesFound() const { return code.CluesFound(); }

private:
	PuzzleCode code;
	SceneNode* leftDoor = nullptr;
	SceneNode* rightDoor = nullptr;
	std::string status;
	float doorOpen = 0.0f;
	bool keypadActive = false;
	bool unlocked = false;
	bool active = false;
};
