#include "StoryDirector.h"
#include "Room.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <utility>

#include <glm/gtc/constants.hpp>

#include "characters/Bullseye.h"
#include "characters/Cat.h"
#include "characters/Character.h"
#include "characters/Humanoid.h"
#include "characters/RCCar.h"
#include "render/Assets.h"
#include "scene/Material.h"
#include "scene/SceneNode.h"
#include "world/PhysicsWorld.h"

namespace {
using namespace RoomSize;

float DistanceXZ(const glm::vec3& a, const glm::vec3& b) { return glm::length(glm::vec2(a.x - b.x, a.z - b.z)); }
float Smooth(float t) { t = std::clamp(t, 0.0f, 1.0f); return t * t * (3.0f - 2.0f * t); }
float Ease(float value, float target, float rate, float dt) { return value + (target - value) * (1.0f - std::exp(-rate * dt)); }

enum Zones { ToyRoomZone, UpperHallZone, StairsZone, GroundZone, BuzzRoomZone, PorchZone, OutsideZone };

const glm::vec3 PennyStreet{6.0f, Ground, 23.3f};
const glm::vec3 PennyHallway{13.3f, 0.0f, 4.5f};
const glm::vec3 InsideEntrance{12.0f, Ground, 8.3f};
const glm::vec3 BuzzDoorCorridor{11.7f, Ground, -5.8f};
const glm::vec3 DoorAim{12.0f, Ground + 2.0f, 9.2f};
const glm::vec3 LaserSpot{12.0f, Ground + 1.2f, 4.4f};
// How close to the point in front of the wardrobe doors someone must stand to try them (Enter).
constexpr float WardrobeReach = 1.6f;
// Closer than this to that point, Penny stands where Bullseye has to stop for his jump.
constexpr float PennyClearance = 1.5f;
// Places beside the corridor walls where the toys wait BEHIND Buzz. Characters are solid: in flight Buzz
// spans x 11.1..12.9 and z 3.4..5.4 at LaserSpot, which leaves no room beside him, and anyone in front
// of him would stand in his line of fire. Bullseye faces the door with his muzzle 1.7 ahead of his
// centre, so the front spots stop at z = 1.0.
const glm::vec3 WaitSpots[4] = {{10.95f, Ground, 1.0f}, {13.0f, Ground, 0.8f}, {10.95f, Ground, -1.0f}, {13.0f, Ground, -1.2f}};
// Where the rescued toys stay in the garden once morning has come.
const glm::vec3 GardenSpots[5] = {{9.2f, Ground, 16.4f}, {10.4f, Ground, 18.2f}, {14.8f, Ground, 17.2f}, {12.4f, Ground, 19.0f}, {13.2f, Ground, 15.6f}};
}

// =============================================================================================
// Set-up
// =============================================================================================
void StoryDirector::Init(Cat* cat, Humanoid* sheriff, Humanoid* cowgirl, Bullseye* horseActor, Buzz* ranger, RCCar* rcCar, PhysicsWorld* world, Hooks hookFunctions)
{
	penny = cat; woody = sheriff; jessie = cowgirl; horse = horseActor; buzz = ranger; car = rcCar; physics = world;
	hooks = std::move(hookFunctions);

	// Doorway nodes of the navigation graph and the zone each lies in.
	nodes = {
		{8.6f, 0.0f, 4.0f},            // 0 Toy Room side of its double door
		{11.4f, 0.0f, 4.0f},           // 1 upper hall, outside the Toy Room
		{15.0f, 0.0f, 2.3f},           // 2 upper hall, at the stair opening
		{15.0f, 0.0f, 0.6f},           // 3 top of the stairs
		{15.0f, Ground + 0.25f, -6.8f},// 4 foot of the stairs
		{15.0f, Ground, -8.1f},        // 5 stair landing
		{12.0f, Ground, -8.0f},        // 6 corridor end
		{11.7f, Ground, -5.8f},        // 7 corridor, at Buzz's door
		{9.3f, Ground, -5.8f},         // 8 Buzz's room, inside the door
		{6.4f, Ground, -5.9f},         // 9 Buzz's room, by the rug
		{12.0f, Ground, 7.6f},         // 10 corridor, at the main entrance
		{12.0f, Ground + 0.45f, 10.6f},// 11 porch
		{12.0f, Ground, 15.5f},        // 12 garden path
	};
	nodeZone = {ToyRoomZone, UpperHallZone, UpperHallZone, StairsZone, StairsZone, GroundZone, GroundZone, GroundZone,
		BuzzRoomZone, BuzzRoomZone, GroundZone, PorchZone, OutsideZone};
	const std::pair<int, int> links[] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 8}, {8, 9}, {7, 10}, {10, 11}, {11, 12}};
	const size_t n = nodes.size();
	const float infinity = std::numeric_limits<float>::max() * 0.25f;
	pathCost.assign(n, std::vector<float>(n, infinity));
	nextHop.assign(n, std::vector<int>(n, -1));
	for (size_t i = 0; i < n; ++i) { pathCost[i][i] = 0.0f; nextHop[i][i] = static_cast<int>(i); }
	for (const auto& [a, b] : links) {
		const float d = glm::distance(nodes[a], nodes[b]);
		pathCost[a][b] = pathCost[b][a] = d;
		nextHop[a][b] = b; nextHop[b][a] = a;
	}
	// Floyd-Warshall: shortest route between every pair of nodes, and the first step of it.
	for (size_t k = 0; k < n; ++k) for (size_t i = 0; i < n; ++i) for (size_t j = 0; j < n; ++j)
		if (pathCost[i][k] + pathCost[k][j] < pathCost[i][j]) {
			pathCost[i][j] = pathCost[i][k] + pathCost[k][j];
			nextHop[i][j] = nextHop[i][k];
		}
}

void StoryDirector::Build(SceneNode& scene, Assets& assets, const HouseRig& rig, const StoryRig& storyProps)
{
	house = rig; props = storyProps;
	auto& iron = assets.Mat("rescue-iron", {0.10f, 0.13f, 0.16f}, 0.65f, 64);
	iron.reflectivity = 0.10f;
	doorLeaf = house.frontDoor->Find("FrontDoorLeaf");
	// The padlock appears on the inside of the main door once it has locked behind Penny.
	lock = house.frontDoor->AddChild("EntrancePadlock");
	lock->local.position = {1.75f, 1.55f, -0.11f};
	lock->AddShape("LockBody", &assets.Cube(), &iron, {0, 0, 0}, {0.24f, 0.28f, 0.12f});
	for (float x : {-0.075f, 0.075f}) lock->AddShape("ShackleUpright", &assets.Cylinder(), &iron, {x, 0.20f, 0}, {0.045f, 0.16f, 0.045f});
	lock->AddShape("ShackleCrown", &assets.Cylinder(), &iron, {0, 0.28f, 0}, {0.045f, 0.15f, 0.045f}, {0, 0, 90});
	// Six preallocated boards use the existing fixed-step rigid-body solver when the door breaks.
	auto& wood = assets.Mat("broken-door-wood", {0.48f, 0.20f, 0.12f}, 0.25f, 24);
	wood.texture = assets.SlotTexture(Assets::FloorSlot); wood.rtTextureSlot = Assets::FloorSlot;
	for (int i = 0; i < 6; ++i) {
		auto* board = scene.AddShape("DoorDebris" + std::to_string(i), &assets.Cube(), &wood,
			{11.25f + (i % 3) * 0.75f, Ground + 0.9f + (i / 3) * 1.7f, 9.3f}, {0.5f, 1.2f, 0.07f});
		board->visible = false; debris.push_back(board);
	}
}

void StoryDirector::Restart(bool story)
{
	sandbox = !story;
	if (Mounted() && hooks.dismount) hooks.dismount();
	for (Character* actor : std::vector<Character*>{woody, jessie, horse, buzz, car}) {
		actor->CancelTransition();
		actor->RestoreRestPose();
		actor->Root()->visible = true;
	}
	woody->SetReach(false); jessie->SetReach(false); woody->Cheer(0); jessie->Cheer(0);
	if (physics) physics->ResetBlocks();
	if (car) car->SetHeadlights(false);
	if (buzz->LaserOn()) buzz->Special();
	elapsed = chestTime = buttonPress = rescueTime = firing = tryTimer = doorShake = morningTime = pullback = 0.0f;
	chestPressed = toysFreed = buzzDoorOpen = rescueRequested = wardrobeOpening = buzzFreed = false;
	doorTried = buzzReady = doorBroken = false;
	rescue = Rescue::Waiting; buzzLeg = 0;
	emergences.clear(); nav.clear(); waitSlot.clear(); controlled = attached = nullptr;
	doorLeaf->visible = doorLeaf->solid = true;
	house.frontDoor->visible = true;
	for (SceneNode* board : debris) board->visible = false;
	if (house.stairDoor) house.stairDoor->local.rotation.y = 90.0f;
	penny->CancelTransition(); penny->Stop();
	penny->SetPose(Cat::Pose::Walk); penny->SetSlope(0.0f); penny->StopLooking();
	enabled = true;

	if (story) {
		// Woody, Jessie and Bullseye wait inside the closed chest; Buzz inside the wardrobe.
		const glm::vec3 c = props.chestCentre;
		emergences = {
			{woody, 0.0f, {c.x - 0.75f, 1.2f, c.z}, {-1.6f, 0.0f, 3.4f}, 10.0f},
			{jessie, 0.6f, {c.x + 0.75f, 1.2f, c.z}, {0.7f, 0.0f, 3.7f}, -5.0f},
			{horse, 1.3f, {c.x, 1.0f, c.z}, {3.4f, 0.0f, 3.1f}, 60.0f},
		};
		const glm::vec3 hidden[3] = {{c.x - 0.75f, -2.6f, c.z}, {c.x + 0.75f, -2.6f, c.z}, {c.x, -3.0f, c.z}};
		for (size_t i = 0; i < emergences.size(); ++i) {
			SceneNode* root = emergences[i].actor->Root();
			root->local.position = hidden[i];
			root->local.rotation.y = i == 2 ? 90.0f : 0.0f;
			root->visible = false;
		}
		buzz->Root()->local.position = props.wardrobeInside;
		buzz->Root()->local.rotation.y = 90.0f;
		buzz->Root()->visible = false;
		penny->Root()->local.position = PennyStreet;
		penny->Root()->local.rotation = {0, 180, 0};
		chestOpen = wardrobeOpen = buzzDoorAngle = 0.0f;
		frontDoorAngle = 80.0f;
		physics->SetExteriorAccess(true);
		hour = 20.75f;
		Enter(GameplayState::PROLOGUE);
	} else {
		chestPressed = toysFreed = buzzFreed = buzzDoorOpen = wardrobeOpening = true;
		rescue = Rescue::Done;
		chestOpen = wardrobeOpen = 1.0f;
		buzzDoorAngle = 95.0f; frontDoorAngle = 0.0f;
		penny->Root()->local.position = PennyHallway;
		penny->Root()->local.rotation = {0, -90, 0};
		physics->SetExteriorAccess(false);
		hour = 20.5f;
		state = GameplayState::FREE_EXPLORE;
	}
	AnimateProps(0.0f);
}

// =============================================================================================
// State machine
// =============================================================================================
bool StoryDirector::Advance(Transition transition)
{
	GameplayState next = state;
	switch (transition) {
	case Transition::ENTERED_HOUSE: if (state == GameplayState::PROLOGUE) next = GameplayState::PUZZLE; break;
	case Transition::PUZZLE_SOLVED: if (state == GameplayState::PUZZLE) next = GameplayState::TOY_CHEST; break;
	case Transition::TOYS_ALIVE: if (state == GameplayState::TOY_CHEST) next = GameplayState::BUZZ_ROOM; break;
	case Transition::BUZZ_ROOM_OPENED: if (state == GameplayState::BUZZ_ROOM) next = GameplayState::WARDROBE; break;
	case Transition::BUZZ_FREED: if (state == GameplayState::WARDROBE) next = GameplayState::FINAL_ESCAPE; break;
	case Transition::LEFT_HOUSE: if (state == GameplayState::FINAL_ESCAPE) next = GameplayState::MORNING; break;
	case Transition::MORNING_COMPLETE: if (state == GameplayState::MORNING) next = GameplayState::FREE_EXPLORE; break;
	}
	if (next == state || sandbox) return false;
	Enter(next);
	return true;
}

void StoryDirector::Enter(GameplayState next)
{
	state = next; elapsed = 0.0f;
	if (next == GameplayState::MORNING) morningStart = hour;
	nav.clear();
	std::cout << "GAMEPLAY " << Title() << "\n";
}

std::string StoryDirector::Title() const
{
	if (sandbox) return "SANDBOX / FREE PLAY";
	switch (state) {
	case GameplayState::PROLOGUE: return "STAGE 0 / NIGHT OUTSIDE THE HOUSE";
	case GameplayState::PUZZLE: return "STAGE 1 / UPSTAIRS HALLWAY - 257 PUZZLE";
	case GameplayState::TOY_CHEST: return "STAGE 2 / THE TOY CHEST";
	case GameplayState::BUZZ_ROOM: return "STAGE 3 / DOWNSTAIRS TO BUZZ'S ROOM";
	case GameplayState::WARDROBE: return "STAGE 4 / CUPBOARD RESCUE";
	case GameplayState::FINAL_ESCAPE: return "STAGE 5 / THE LOCKED MAIN DOOR";
	case GameplayState::MORNING: return "STAGE 6 / MORNING ESCAPE";
	case GameplayState::FREE_EXPLORE: return "STAGE 7 / FREE EXPLORATION";
	}
	return {};
}

std::string StoryDirector::Objective() const
{
	switch (state) {
	case GameplayState::PROLOGUE: return "Explore outside, then walk in through the open front door";
	case GameplayState::PUZZLE: return "Go upstairs: find the 3 clues and enter the code at the Toy Room door";
	case GameplayState::TOY_CHEST:
		return toysFreed ? "The toys are alive!" : chestPressed ? "The chest is opening..." : "Press the glowing red button on the toy chest (Enter)";
	case GameplayState::BUZZ_ROOM: return "Lead the toys downstairs and open the door on the left of the corridor (Enter)";
	case GameplayState::WARDROBE:
		if (wardrobeOpening) return "Buzz is free!";
		if (Controls(jessie) || Controls(horse)) return "Jessie: R mount Bullseye, ride beside the wardrobe, L jump to reach the knob";
		if (rescue != Rescue::Waiting && Controls(penny) && DistanceXZ(penny->Root()->local.position, props.wardrobeFront) < PennyClearance)
			return "Step back from the wardrobe so Bullseye can reach the doors";
		return rescue == Rescue::Waiting ? "Walk Penny right up to the glowing wardrobe and press Enter" : "Jessie rides Bullseye to the wardrobe...";
	case GameplayState::FINAL_ESCAPE:
		if (doorBroken) return "The way is open: take everyone outside";
		if (!doorTried) return "Lead everyone to the main entrance and try the door (Enter)";
		if (Controls(buzz)) return "Buzz: fly in front of the door, aim (Z/X) and fire the laser (L)";
		return buzzReady ? "Stand clear and press L: Buzz fires his laser at the door" : "Locked! Buzz flies into position...";
	case GameplayState::MORNING: return "Morning is coming. Everyone outside!";
	case GameplayState::FREE_EXPLORE:
		return sandbox ? "Free play: select any toy" : "Explore the neighbourhood. 1-5 select a toy: Buzz flies (Q/E), Jessie rides Bullseye (R)";
	}
	return {};
}

std::string StoryDirector::Caption() const
{
	switch (state) {
	case GameplayState::PROLOGUE: return "Night. Penny arrives at the abandoned house; the front door stands open.";
	case GameplayState::PUZZLE: return "The entrance has locked behind her. Upstairs, a combination lock guards the Toy Room.";
	case GameplayState::TOY_CHEST: return "An old wooden toy chest with a strange glowing red button.";
	case GameplayState::BUZZ_ROOM: return "Woody, Jessie and Bullseye follow Penny back down the stairs.";
	case GameplayState::WARDROBE: return "Something inside the wardrobe glows and rattles.";
	case GameplayState::FINAL_ESCAPE: return "The main door is locked. There is no normal way out.";
	case GameplayState::MORNING: return "The night lifts. The fog thins and the sun rises over the house.";
	case GameplayState::FREE_EXPLORE: return "The toys are safe. The story stays open for exploration.";
	}
	return {};
}

bool StoryDirector::Available(const Character* actor) const
{
	if (sandbox || actor == penny) return true;
	if (actor == buzz) return buzzFreed;
	return toysFreed;
}

bool StoryDirector::Hidden(const Character* actor) const
{
	return actor && !actor->Root()->visible;
}

bool StoryDirector::Scripted(const Character* actor) const
{
	if (!actor) return false;
	for (const Emergence& e : emergences) if (e.actor == actor && (e.phase == 1 || e.phase == 2)) return true;
	return actor == buzz && wardrobeOpening && !buzzFreed; // standing in the wardrobe, then his first flight
}

bool StoryDirector::Busy(const Character* actor) const
{
	if (!actor || !enabled || sandbox) return false;
	if (state == GameplayState::WARDROBE && (actor == jessie || actor == horse))
		return rescue != Rescue::Waiting && rescue != Rescue::BuzzFlight && rescue != Rescue::Done;
	// Buzz keeps right of way while he flies to his firing position and while he holds it.
	if (state == GameplayState::FINAL_ESCAPE && actor == buzz) return doorTried && !doorBroken;
	return false;
}

bool StoryDirector::FollowersActive() const
{
	return enabled && !sandbox && state >= GameplayState::BUZZ_ROOM;
}

void StoryDirector::Pause()
{
	for (Character* actor : std::vector<Character*>{woody, jessie, horse, buzz, car, penny}) actor->Stop();
	if (buzz->LaserOn() && !Controls(buzz)) buzz->Special();
}

void StoryDirector::SetControlled(Character* actor, Character* passenger)
{
	if (controlled == actor && attached == passenger) return;
	controlled = actor; attached = passenger;
	if (actor) nav.erase(actor);
}

Character* StoryDirector::Leader() const
{
	// The toys accompany Penny. A toy the player drives leaves the group and nobody else changes.
	return penny;
}

bool StoryDirector::Cinematic(glm::vec3& position, glm::vec3& target) const
{
	if (!enabled || state != GameplayState::MORNING || pullback <= 0.0f) return false;
	position = {2.0f, 3.5f, 40.0f};
	target = {8.5f, Ground + 3.6f, 2.0f};
	return true;
}

bool StoryDirector::Glow(glm::vec3& position, glm::vec3& color, float& intensity) const
{
	if (sandbox) return false;
	const float pulse = 0.55f + 0.45f * std::sin(elapsed * 3.5f);
	if (!chestPressed) {
		position = props.chestButton->WorldPosition() + glm::vec3(0, 0.45f, 0.3f);
		color = {1.0f, 0.18f, 0.08f}; intensity = 1.1f * pulse;
		return true;
	}
	if (!wardrobeOpening && state >= GameplayState::BUZZ_ROOM) {
		position = props.wardrobeInside + glm::vec3(1.1f, 1.6f, 0.0f);
		color = {0.3f, 1.0f, 0.5f}; intensity = 0.7f * pulse;
		return true;
	}
	return false;
}

// =============================================================================================
// Interaction (Enter)
// =============================================================================================
std::string StoryDirector::Interact(Character* actor)
{
	if (!actor || sandbox) return {};
	const glm::vec3 p = actor->Root()->WorldPosition();
	const bool downstairs = p.y < Slab;
	switch (state) {
	case GameplayState::PROLOGUE:
		return DistanceXZ(p, {12.0f, 0.0f, 9.2f}) < 3.0f ? "The front door is open. Walk inside." : "Explore the garden, then walk in through the front door.";
	case GameplayState::TOY_CHEST:
		if (chestPressed) return {};
		if (!downstairs && DistanceXZ(p, props.chestCentre) < 3.2f) {
			PressChest();
			return "Penny presses the big red wind-up button. The chest springs open!";
		}
		return "The glowing toy chest stands in the middle of the Toy Room.";
	case GameplayState::BUZZ_ROOM:
		if (downstairs && DistanceXZ(p, BuzzDoorCorridor) < 2.3f) {
			buzzDoorOpen = true;
			Advance(Transition::BUZZ_ROOM_OPENED);
			return "Penny opens the door: an ordinary bedroom, with a big wooden wardrobe at the back.";
		}
		return "Go downstairs. The door is on the left of the corridor.";
	case GameplayState::WARDROBE:
		if (wardrobeOpening) return {};
		// Only right in front of the doors (wardrobeFront is 1.05 in front of them), never across the room.
		if (!downstairs || DistanceXZ(p, props.wardrobeFront) > WardrobeReach)
			return Zone(p) == BuzzRoomZone ? "Walk right up to the wardrobe doors, then press Enter." : std::string();
		if (actor == horse && Mounted()) {
			horse->Jump(); jessie->SetReach(true); rescueTime = 0.0f;
			return "Bullseye jumps and Jessie reaches for the knob!";
		}
		if (actor == jessie) return "Too high, even for Jessie. Mount Bullseye (R) and jump (L) beside the wardrobe.";
		if (rescueRequested) return "Jessie and Bullseye are on their way to the wardrobe.";
		rescueRequested = true;
		return "The knob is too high for Penny. Jessie and Bullseye will help!";
	case GameplayState::FINAL_ESCAPE:
		if (doorBroken || !downstairs || DistanceXZ(p, InsideEntrance) > 2.6f) return doorTried ? std::string() : "Lead everyone to the main entrance.";
		if (!doorTried) { TryDoor(); return "Penny tries the main door. It is locked: there is no normal way out. Buzz looks at the door."; }
		if (RequestLaser()) return "Buzz fires his laser!";
		return {};
	default:
		return {};
	}
}

bool StoryDirector::RequestLaser()
{
	if (state != GameplayState::FINAL_ESCAPE || !doorTried || doorBroken || !buzzReady || Controls(buzz)) return false;
	if (!buzz->LaserOn()) buzz->Special();
	std::cout << "ESCAPE laser fired at the entrance\n";
	return true;
}

void StoryDirector::PressChest()
{
	chestPressed = true; chestTime = 0.0f; buttonPress = 1.0f;
	std::cout << "STORY chest button pressed\n";
}

void StoryDirector::OpenWardrobe()
{
	if (wardrobeOpening) return;
	wardrobeOpening = true;
	buzz->Root()->visible = true;
	std::cout << "STORY wardrobe opened\n";
}

void StoryDirector::TryDoor()
{
	doorTried = true; doorShake = 0.8f;
	// Front-most character -> front-most spot (largest z), and so on back down the corridor.
	std::vector<Character*> group = {woody, horse, jessie, penny};
	std::sort(group.begin(), group.end(), [](const Character* a, const Character* b) {
		return a->Root()->WorldPosition().z > b->Root()->WorldPosition().z; });
	std::vector<int> spots = {0, 1, 2, 3};
	std::sort(spots.begin(), spots.end(), [](int a, int b) { return WaitSpots[a].z > WaitSpots[b].z; });
	waitSlot.clear();
	for (size_t i = 0; i < group.size(); ++i) waitSlot[group[i]] = spots[i];
	std::cout << "STORY main door is locked\n";
}

void StoryDirector::BreakDoor()
{
	doorBroken = true;
	house.frontDoor->visible = false; doorLeaf->solid = false;
	physics->SetExteriorAccess(true);
	for (size_t i = 0; i < debris.size(); ++i) {
		debris[i]->visible = true;
		const float spread = static_cast<float>(i % 3) - 1.0f;
		physics->PushBlock(debris[i], {spread * 1.6f, 1.4f + 0.3f * static_cast<float>(i / 3), 4.5f + 0.4f * static_cast<float>(i)},
			{80.0f * spread + 30.0f, 25.0f, 70.0f});
	}
	if (buzz->LaserOn() && !Controls(buzz)) buzz->Special();
	std::cout << "ESCAPE real laser broke entrance door\n";
}

// =============================================================================================
// Per-frame update
// =============================================================================================
void StoryDirector::AnimateProps(float dt)
{
	// Toy chest: lid on its back hinge, the pressed button springs back, the wind-up key spins.
	props.chestLid->local.rotation.x = -105.0f * Smooth(chestOpen);
	buttonPress = std::max(0.0f, buttonPress - dt * 2.0f);
	props.chestButton->local.scale.y = 1.0f - 0.55f * std::sin(buttonPress * glm::pi<float>() * 0.5f);
	if (chestPressed && chestOpen < 1.0f) props.chestKey->local.rotation.x += 900.0f * dt;
	const float pulse = 0.55f + 0.45f * std::sin(elapsed * 3.5f);
	props.buttonGlow->emissive = chestPressed && !sandbox ? glm::vec3(0.12f, 0.01f, 0.0f) : glm::vec3(0.55f, 0.04f, 0.0f) * (sandbox ? 0.5f : pulse);

	// Buzz's room door.
	buzzDoorAngle = dt > 0.0f ? Ease(buzzDoorAngle, buzzDoorOpen ? 95.0f : 0.0f, 3.0f, dt) : buzzDoorAngle;
	props.buzzDoor->local.rotation.y = buzzDoorAngle;

	// Wardrobe: rattles every few seconds while Buzz is trapped, then both doors swing open.
	if (wardrobeOpening) wardrobeOpen = std::min(1.0f, wardrobeOpen + dt / 0.9f);
	const float rattleAngle = !wardrobeOpening && std::fmod(elapsed, 4.5f) < 0.5f ? 2.0f * std::sin(elapsed * 45.0f) : 0.0f;
	const float swing = 100.0f * Smooth(wardrobeOpen) + rattleAngle;
	props.wardrobeLeft->local.rotation.y = -swing;
	props.wardrobeRight->local.rotation.y = swing;
	const bool hint = !wardrobeOpening && !sandbox;
	props.wardrobeGlow->emissive = hint ? glm::vec3(0.25f, 0.95f, 0.45f) * (0.5f + 0.5f * std::sin(elapsed * 2.3f)) : glm::vec3(0.0f);
	props.wardrobeGlow->opacity = hint ? 0.85f : 0.0f;

	// Main door: open in the prologue, then closed and padlocked; it shakes when Penny tries it.
	if (!doorBroken) {
		const float target = state == GameplayState::PROLOGUE && !sandbox ? 80.0f : 0.0f;
		frontDoorAngle = dt > 0.0f ? Ease(frontDoorAngle, target, 2.5f, dt) : frontDoorAngle;
		doorShake = std::max(0.0f, doorShake - dt);
		house.frontDoor->local.rotation.y = frontDoorAngle + (doorShake > 0.0f ? 1.5f * std::sin(elapsed * 55.0f) : 0.0f);
		lock->visible = !sandbox && state != GameplayState::PROLOGUE;
	}
}

void StoryDirector::Update(float dt)
{
	if (!enabled || sandbox) return;
	elapsed += dt;
	AnimateProps(dt);

	// The night deepens as the story advances; the morning stage owns the clock itself.
	static const float nightHours[] = {20.75f, 21.25f, 22.0f, 22.75f, 23.25f, 23.6f};
	if (state <= GameplayState::FINAL_ESCAPE) hour = Ease(hour, nightHours[static_cast<int>(state)], 0.4f, dt);

	switch (state) {
	case GameplayState::PROLOGUE: {
		const glm::vec3 p = penny->Root()->local.position;
		if (p.y < Slab && p.z < 8.0f && p.x > CorridorLeft && p.x < CorridorRight) {
			physics->SetExteriorAccess(false);
			Advance(Transition::ENTERED_HOUSE);
			std::cout << "STORY the main door closed and locked behind Penny\n";
		}
		break;
	}
	case GameplayState::PUZZLE:
		break;
	case GameplayState::TOY_CHEST:
		UpdateChest(dt);
		break;
	case GameplayState::BUZZ_ROOM:
		UpdateFollowers(dt, {});
		break;
	case GameplayState::WARDROBE:
		UpdateRescue(dt);
		break;
	case GameplayState::FINAL_ESCAPE:
		UpdateEntrance(dt);
		break;
	case GameplayState::MORNING: {
		morningTime += dt;
		// 23:36 -> 07:00 over sixteen seconds (the clock wraps through midnight).
		hour = std::fmod(morningStart + (31.0f - morningStart) * Smooth(morningTime / 16.0f), 24.0f);
		// Out through the broken door, each to their own place on the lawn. (A formation behind Penny
		// would point back at the house and park the toys on the porch steps, in everyone else's way.)
		Gather(dt, true);
		bool everyone = true;
		for (const Character* actor : std::vector<const Character*>{penny, woody, jessie, horse, buzz}) {
			const glm::vec3 p = actor->Root()->WorldPosition();
			everyone = everyone && p.z > HouseFront + 3.0f && p.y < Slab;
		}
		if (everyone || morningTime > 35.0f) pullback += dt;
		if (pullback > 7.5f && morningTime > 16.0f) Advance(Transition::MORNING_COMPLETE);
		break;
	}
	case GameplayState::FREE_EXPLORE:
		hour = Ease(hour, 8.0f, 0.05f, dt); // the morning carries on slowly
		Gather(dt);
		break;
	}
}

void StoryDirector::UpdateChest(float dt)
{
	if (!chestPressed) return;
	chestTime += dt;
	chestOpen = std::clamp((chestTime - 0.35f) / 1.1f, 0.0f, 1.0f);
	bool landed = true;
	for (Emergence& e : emergences) {
		Character* actor = e.actor;
		if (e.phase == 0 && chestOpen > 0.75f && chestTime > 1.2f + e.delay) {
			// Climb up out of the chest, then hop down in front of it.
			actor->Root()->visible = true;
			actor->StartTransition(e.rise, e.heading, 1.0f);
			e.phase = 1;
		} else if (e.phase == 1 && !actor->InTransition()) {
			actor->StartTransition(e.land, e.heading, 0.8f);
			e.phase = 2;
		} else if (e.phase == 2 && !actor->InTransition()) {
			e.phase = 3;
			if (actor == woody) woody->Cheer(2.5f);
			if (actor == jessie) jessie->Cheer(2.5f);
			if (actor == horse) horse->Jump();
		}
		landed = landed && e.phase == 3;
	}
	if (landed && !toysFreed) { toysFreed = true; rescueTime = 0.0f; std::cout << "STORY the toys are alive\n"; }
	if (toysFreed && (rescueTime += dt) > 2.8f) Advance(Transition::TOYS_ALIVE);
}

void StoryDirector::UpdateRescue(float dt)
{
	const bool teamAuto = !Controls(jessie) && !Controls(horse);
	// The rescue starts only when someone has walked up to the wardrobe and tried its doors (Enter, see
	// Interact); merely entering the bedroom does nothing.
	if (rescue == Rescue::Waiting && teamAuto && rescueRequested) {
		rescue = Rescue::Mounting;
		std::cout << "STORY Jessie and Bullseye start the wardrobe rescue\n";
	}
	// Live control: the player can do it with the same actions (R mount, ride, L or Enter jump).
	const float reachable = DistanceXZ(horse->Root()->local.position, props.wardrobeFront);
	if (!wardrobeOpening && Mounted() && reachable < 1.4f && horse->JumpLift() > 0.45f) {
		jessie->SetReach(true);
		OpenWardrobe();
	}
	if (wardrobeOpening && wardrobeOpen > 0.85f && rescue != Rescue::Jumping && rescue != Rescue::StepAside
		&& rescue != Rescue::BuzzFlight && rescue != Rescue::Done) {
		rescue = Rescue::BuzzFlight; buzzLeg = 0;
	}

	std::vector<Character*> busy;
	if (rescue != Rescue::Waiting && rescue != Rescue::Done && rescue != Rescue::BuzzFlight) busy = {jessie, horse};
	UpdateFollowers(dt, busy);

	const glm::vec3 spot = props.wardrobeFront;
	// Characters are solid to each other, so Penny must leave the spot in front of the doors for Bullseye.
	// The story moves her only while the player is not driving her (the objective asks the player otherwise).
	if ((rescue == Rescue::Mounting || rescue == Rescue::Riding || rescue == Rescue::Aligning) && !Controls(penny)
		&& DistanceXZ(penny->Root()->local.position, spot) < PennyClearance)
		Navigate(penny, spot + glm::vec3(2.2f, 0.0f, -1.8f), dt, 2.0f, 0.3f);
	switch (rescue) {
	case Rescue::Mounting: {
		if (!teamAuto) break;
		if (Mounted()) { if (!jessie->InTransition()) rescue = Rescue::Riding; break; }
		// Bullseye waits; Jessie walks up to him (the mount hop itself is a scripted transition).
		horse->Stop();
		Navigate(jessie, horse->Root()->local.position, dt, 2.6f, 1.6f);
		if (DistanceXZ(jessie->Root()->local.position, horse->Root()->local.position) < 2.0f && hooks.mount) hooks.mount();
		break;
	}
	case Rescue::Riding:
		// He walks up head first, and his muzzle (1.7 ahead of his centre) now touches the doors before
		// his centre reaches the spot: arriving within 1.0 is close enough (the knob is in reach under 1.4).
		if (teamAuto && Navigate(horse, spot, dt, 2.4f, 1.0f)) rescue = Rescue::Aligning;
		break;
	case Rescue::Aligning:
		if (teamAuto && horse->TurnTowardsHeading(0.0f, dt)) {
			horse->Jump(); jessie->SetReach(true); rescueTime = 0.0f;
			rescue = Rescue::Jumping;
		}
		break;
	case Rescue::Jumping:
		rescueTime += dt;
		if (horse->JumpLift() > 0.45f) OpenWardrobe();
		if (rescueTime > 1.3f) { jessie->SetReach(false); rescue = Rescue::StepAside; rescueTime = 0.0f; }
		break;
	case Rescue::StepAside:
		// Bullseye backs away so Buzz has room to fly out.
		rescueTime += dt;
		if (Navigate(horse, spot + glm::vec3(1.2f, 0.0f, 2.3f), dt, 2.0f, 0.25f) || rescueTime > 3.0f) {
			rescue = Rescue::BuzzFlight; buzzLeg = 0;
		}
		break;
	case Rescue::BuzzFlight: {
		jessie->SetReach(false);
		// Buzz lifts off inside the wardrobe, flies out over the rug and lands: flight shown at once.
		const glm::vec3 inside = props.wardrobeInside;
		const glm::vec3 legs[4] = {inside + glm::vec3(0.0f, 0.8f, 0.0f), inside + glm::vec3(2.0f, 1.4f, -1.2f),
			{5.6f, Ground + 1.7f, -7.0f}, {5.2f, Ground + 0.02f, -6.6f}};
		buzz->Root()->visible = true;
		if (buzzLeg < 4 && buzz->FollowWaypoint(legs[buzzLeg], dt, 2.2f)) ++buzzLeg;
		if (buzzLeg >= 4) {
			buzz->Stop(); buzz->Cheer(2.0f);
			buzzFreed = true; rescue = Rescue::Done;
			std::cout << "STORY Buzz is free\n";
			Advance(Transition::BUZZ_FREED);
		}
		break;
	}
	default:
		break;
	}
}

void StoryDirector::UpdateEntrance(float dt)
{
	// The laser counts only while its nearest hit is the real door leaf, whoever is flying Buzz.
	if (!doorBroken) {
		if (buzz->LaserOn() && physics->LastLaserHit() == doorLeaf) firing += dt; else firing = 0.0f;
		if (firing > 0.65f) BreakDoor();
	}
	if (doorBroken) {
		UpdateFollowers(dt, {});
		const glm::vec3 p = Leader()->Root()->WorldPosition();
		if (p.z > HouseFront + 0.4f && p.y < Slab) Advance(Transition::LEFT_HOUSE);
		return;
	}
	if (!doorTried) {
		UpdateFollowers(dt, {});
		// Penny tries the door when she stands at it (Enter does the same at once).
		tryTimer = DistanceXZ(Leader()->Root()->WorldPosition(), InsideEntrance) < 1.6f ? tryTimer + dt : 0.0f;
		if (tryTimer > 1.0f) TryDoor();
		return;
	}
	// The door is locked: everyone waits beside the walls while Buzz flies into position and aims.
	for (Character* actor : std::vector<Character*>{woody, horse, jessie, penny}) {
		if (Controls(actor) || (actor == jessie && Mounted())) continue;
		const auto slot = waitSlot.find(actor);
		if (Navigate(actor, WaitSpots[slot != waitSlot.end() ? slot->second : 0], dt, 2.4f, 0.4f)) actor->TurnTowardsHeading(0.0f, dt);
	}
	if (!Controls(buzz)) {
		if (!buzzReady && Navigate(buzz, LaserSpot, dt, 2.2f, 0.15f)) {
			buzzReady = true;
			std::cout << "STORY Buzz is in position\n";
		}
		if (buzzReady || DistanceXZ(buzz->Root()->local.position, LaserSpot) < 1.5f) buzz->AimAt(DoorAim);
	}
}

void StoryDirector::Gather(float dt, bool withPenny)
{
	int slot = 0;
	for (Character* actor : std::vector<Character*>{woody, jessie, horse, buzz, penny}) {
		const int index = slot++;
		if (Controls(actor) || (actor == jessie && Mounted()) || (actor == penny && !withPenny)) continue;
		Navigate(actor, GardenSpots[index], dt, actor == penny ? 2.0f : 2.4f, 0.3f);
	}
}

void StoryDirector::UpdateFollowers(float dt, const std::vector<Character*>& exclude)
{
	Character* lead = Leader();
	// Each toy owns a fixed formation slot, so taking one toy over never reshuffles the others.
	std::vector<std::pair<Character*, int>> group;
	int slot = 0;
	for (Character* actor : std::vector<Character*>{woody, jessie, horse, buzz}) {
		const int mine = slot++;
		if (actor == lead || Controls(actor) || Hidden(actor) || Scripted(actor) || actor->InTransition()) continue;
		if (std::find(exclude.begin(), exclude.end(), actor) != exclude.end()) continue;
		if ((actor == jessie && Mounted()) || (actor == buzz && !buzzFreed) || !toysFreed) continue;
		group.push_back({actor, mine});
	}
	const glm::vec3 lp = lead->Root()->WorldPosition();
	const glm::vec3 back = -lead->Forward();
	const glm::vec3 side(back.z, 0.0f, -back.x);
	const int leadZone = Zone(lp);
	for (const auto& [actor, i] : group) {
		// Formation slots: pairs behind the leader, alternating left and right. When the space behind
		// her is another room (she stands in a doorway), the formation turns to her side or front.
		glm::vec3 target = lp;
		for (const glm::vec3& d : {back, side, -side, -back}) {
			const glm::vec3 across(d.z, 0.0f, -d.x);
			const glm::vec3 spot = lp + d * (1.7f + 1.3f * static_cast<float>(i / 2)) + across * (i % 2 == 0 ? -0.9f : 0.9f);
			if (Zone(spot) == leadZone) { target = spot; break; }
		}
		target.y = PhysicsWorld::FloorHeight({target.x, lp.y, target.z});
		if (actor == buzz && state == GameplayState::MORNING) target.y += 1.4f; // Buzz flies out into the morning
		Nav& n = nav[actor];
		const glm::vec3 p = actor->Root()->local.position;
		const float d = actor->CanFly() ? glm::distance(p, target) : DistanceXZ(p, target);
		n.moving = n.moving ? d > 0.3f : d > 1.3f;
		if (!n.moving) { actor->Stop(); continue; }
		Navigate(actor, target, dt, std::clamp(d * 0.9f, 1.8f, 3.6f), 0.3f);
	}
}

// =============================================================================================
// Navigation
// =============================================================================================
int StoryDirector::Zone(const glm::vec3& p)
{
	// The porch is its own zone: it is fenced by railings and left only down its steps (node 12), so
	// nobody heads for a spot in the garden straight across a railing.
	if (p.z > HouseFront) return p.x >= 7.6f && p.x <= 16.4f && p.z < 13.0f ? PorchZone : OutsideZone;
	if (p.x >= StairLeft - 0.05f && p.x <= StairRight + 0.05f && p.z >= StairBottomZ && p.z < StairTopZ) return StairsZone;
	if (p.y < Slab) return p.x < CorridorLeft ? BuzzRoomZone : GroundZone;
	return p.x < HalfWidth ? ToyRoomZone : UpperHallZone;
}

int StoryDirector::NearestNode(const glm::vec3& p, int zone) const
{
	int best = -1; float distance = std::numeric_limits<float>::max();
	for (size_t i = 0; i < nodes.size(); ++i) {
		if (nodeZone[i] != zone) continue;
		const float d = DistanceXZ(p, nodes[i]);
		if (d < distance) { distance = d; best = static_cast<int>(i); }
	}
	return best;
}

// Local avoidance: characters are solid to each other, so a walker heads past anyone standing in its
// path instead of pressing into them. Each body within 2.6 ahead whose centre is closer to the walking
// line than the two bodies' radii adds a sideways pull away from it; the pull fades with distance.
glm::vec3 StoryDirector::Steer(const Character* actor, const glm::vec3& waypoint) const
{
	auto radius = [&](const Character* c) { return c == horse ? 0.9f : c == penny ? 0.6f : c == buzz && buzz->Root()->local.position.y > PhysicsWorld::FloorHeight(buzz->Root()->local.position) + 0.06f ? 0.95f : 0.5f; };
	const glm::vec3 p = actor->Root()->local.position;
	glm::vec2 dir(waypoint.x - p.x, waypoint.z - p.z);
	const float distance = glm::length(dir);
	if (distance < 0.3f) return waypoint;
	dir /= distance;
	glm::vec2 steer(0.0f);
	for (const Character* other : std::vector<const Character*>{penny, woody, jessie, horse, buzz}) {
		if (other == actor || Hidden(other) || (other == jessie && Mounted()) || (actor == jessie && Mounted() && other == horse)) continue;
		const glm::vec3 q = other->Root()->local.position;
		if (std::abs(q.y - p.y) > 1.6f) continue; // one flies above the other
		const glm::vec2 rel(q.x - p.x, q.z - p.z);
		const float along = glm::dot(rel, dir);
		if (along <= 0.0f || along > std::min(2.6f, distance + 0.5f)) continue;
		const glm::vec2 side = rel - dir * along;
		const float lateral = glm::length(side);
		const float clearance = radius(actor) + radius(other) + 0.15f;
		if (lateral >= clearance) continue;
		// Away from the other body; when it stands dead ahead, pass it on the left.
		const glm::vec2 away = lateral > 1e-3f ? -side / lateral : glm::vec2(-dir.y, dir.x);
		steer += away * (clearance - lateral) * (1.0f - along / 2.6f);
	}
	if (glm::dot(steer, steer) < 1e-6f) return waypoint;
	const glm::vec2 ahead = glm::vec2(p.x, p.z) + dir * std::min(distance, 1.2f) + steer * 1.8f;
	return {ahead.x, waypoint.y, ahead.y};
}

bool StoryDirector::MoveTo(Character* actor, const glm::vec3& target, float dt, float speed, float arrive)
{
	const glm::vec3 p = actor->Root()->local.position;
	const float d = actor->CanFly() ? glm::distance(p, target) : DistanceXZ(p, target);
	if (d <= arrive) { actor->Stop(); return true; }
	actor->FollowWaypoint(Steer(actor, target), dt, speed);
	return false;
}

bool StoryDirector::Navigate(Character* actor, const glm::vec3& target, float dt, float speed, float arrive)
{
	Nav& n = nav[actor];
	const glm::vec3 p = actor->Root()->local.position;
	const int from = Zone(p), to = Zone(target);
	if (from == to) { n.node = -1; return MoveTo(actor, target, dt, speed, arrive); }
	const int goal = NearestNode(target, to);
	if (goal < 0) return MoveTo(actor, target, dt, speed, arrive);
	if (n.node < 0 || n.goal != goal) {
		// Enter the graph at the node of this zone with the cheapest total route.
		n.goal = goal; n.node = -1;
		float best = std::numeric_limits<float>::max();
		for (size_t i = 0; i < nodes.size(); ++i) {
			if (nodeZone[i] != from) continue;
			const float cost = DistanceXZ(p, nodes[i]) + pathCost[i][static_cast<size_t>(goal)];
			if (cost < best) { best = cost; n.node = static_cast<int>(i); }
		}
		if (n.node < 0) return MoveTo(actor, target, dt, speed, arrive);
	}
	// A doorway node counts as passed within reach of the body's centre. Bullseye's centre sits 1.7 behind
	// his solid muzzle, so a wall or an open door leaf can stop it short of a tight 0.4.
	const float reach = actor == horse ? 0.9f : 0.4f;
	if (DistanceXZ(p, nodes[static_cast<size_t>(n.node)]) < reach) {
		if (n.node == goal) { n.node = -1; return MoveTo(actor, target, dt, speed, arrive); }
		n.node = nextHop[static_cast<size_t>(n.node)][static_cast<size_t>(goal)];
	}
	actor->FollowWaypoint(Steer(actor, nodes[static_cast<size_t>(n.node)]), dt, speed);
	return false;
}
