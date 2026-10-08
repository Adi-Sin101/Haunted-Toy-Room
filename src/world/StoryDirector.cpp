#include "StoryDirector.h"

#include <iostream>
#include <utility>
#include <vector>

#include "characters/Bullseye.h"
#include "characters/Character.h"
#include "characters/Humanoid.h"
#include "characters/RCCar.h"
#include "world/PhysicsWorld.h"

void StoryDirector::Init(Humanoid* w, Humanoid* j, Bullseye* h, Buzz* b, RCCar* c,
	PhysicsWorld* p, std::function<void()> d)
{
	woody = w;
	jessie = j;
	horse = h;
	buzz = b;
	car = c;
	physics = p;
	dismount = std::move(d);
	Restart(false);
}

void StoryDirector::Pause()
{
	for (Character* actor : std::vector<Character*>{woody, jessie, horse, buzz, car}) {
		if (actor) actor->Stop();
	}
	if (buzz && buzz->LaserOn()) buzz->Special();
}

void StoryDirector::Restart(bool mounted)
{
	if (mounted && dismount) dismount();
	for (Character* actor : std::vector<Character*>{woody, jessie, horse, buzz, car}) {
		if (!actor) continue;
		actor->StartTransition(actor->HomePosition(), actor->HomeHeading(), 0.01f);
		actor->Animate(0.02f, 0);
		actor->RestoreRestPose();
	}
	if (physics) physics->ResetBlocks();
	if (car) car->SetHeadlights(false);
	if (buzz && buzz->LaserOn()) buzz->Special();
	enabled = true;
	Enter(GameplayState::PROLOGUE);
}

void StoryDirector::BeginGameplay()
{
	Advance(Transition::ARRIVAL_COMPLETE);
}

bool StoryDirector::Advance(Transition transition)
{
	GameplayState next = state;
	switch (transition) {
	case Transition::ARRIVAL_COMPLETE:
		if (state == GameplayState::PROLOGUE) next = GameplayState::PUZZLE;
		break;
	case Transition::PUZZLE_SOLVED:
		if (state == GameplayState::PUZZLE) next = GameplayState::TOY_RESCUE;
		break;
	case Transition::TOYS_FREED:
		if (state == GameplayState::TOY_RESCUE) next = GameplayState::BUZZ_RESCUE;
		break;
	case Transition::BUZZ_FREED:
		if (state == GameplayState::BUZZ_RESCUE) next = GameplayState::FINAL_ESCAPE;
		break;
	case Transition::ESCAPE_COMPLETE:
		if (state == GameplayState::FINAL_ESCAPE) next = GameplayState::WIN;
		break;
	}
	if (next == state) return false;
	Enter(next);
	return true;
}

void StoryDirector::Enter(GameplayState next)
{
	state = next;
	std::cout << "GAMEPLAY " << Title() << "\n";
}

std::string StoryDirector::Title() const
{
	switch (state) {
	case GameplayState::PROLOGUE: return "STAGE 0 / PROLOGUE - ARRIVAL";
	case GameplayState::PUZZLE: return "STAGE 1 / ENTRANCE - PUZZLE";
	case GameplayState::TOY_RESCUE: return "STAGE 2 / TOY RESCUE";
	case GameplayState::BUZZ_RESCUE: return "STAGE 3 / BUZZ ROOM";
	case GameplayState::FINAL_ESCAPE: return "STAGE 4 / FINAL ESCAPE";
	case GameplayState::WIN: return "WIN / ENDING";
	}
	return {};
}

std::string StoryDirector::Objective() const
{
	switch (state) {
	case GameplayState::PROLOGUE: return "Penny's arrival";
	case GameplayState::PUZZLE: return "Find the code: 3 clues";
	case GameplayState::TOY_RESCUE: return "Free the Toys";
	case GameplayState::BUZZ_RESCUE: return "Free Buzz";
	case GameplayState::FINAL_ESCAPE: return "GET EVERYONE OUT";
	case GameplayState::WIN: return "THE TOYS ARE SAFE\nYOU ESCAPED";
	}
	return {};
}

std::string StoryDirector::Caption() const
{
	switch (state) {
	case GameplayState::PROLOGUE: return "Penny arrives at the abandoned house as night falls.";
	case GameplayState::PUZZLE: return "Inspect the toy train, old clock, and colored blocks, then enter their numbers at the keypad.";
	case GameplayState::TOY_RESCUE: return "Woody, Jessie and Bullseye are trapped inside the Toy Room.";
	case GameplayState::BUZZ_RESCUE: return "Woody points Penny toward the room where Buzz is trapped.";
	case GameplayState::FINAL_ESCAPE: return "The house is hostile. Get the rescued toys back to the main entrance.";
	case GameplayState::WIN: return "The toys escaped. The house falls dark behind them.";
	}
	return {};
}
