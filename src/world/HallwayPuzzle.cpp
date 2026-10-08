#include "HallwayPuzzle.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "Room.h"
#include "render/Assets.h"
#include "scene/Material.h"
#include "scene/SceneNode.h"
#include "world/House.h"

namespace {
constexpr glm::vec3 TrainPosition{15.7f, 0.0f, 5.85f};
constexpr glm::vec3 BlocksPosition{11.4f, 0.0f, 3.0f};
constexpr glm::vec3 ClockPosition{13.3f, 1.95f, 1.15f};
constexpr glm::vec3 KeypadPosition{12.3f, 1.45f, 6.82f};

Material& Wood(Assets& assets, const char* name, glm::vec3 color)
{
	Material& material = assets.Mat(name, color, 0.28f, 28.0f);
	material.texture = assets.SlotTexture(Assets::FloorSlot);
	material.rtTextureSlot = Assets::FloorSlot;
	material.uvScale = {0.8f, 0.8f};
	return material;
}

SceneNode* Box(SceneNode& parent, Assets& assets, const char* name, Material& material,
	const glm::vec3& position, const glm::vec3& size, bool solid = false)
{
	SceneNode* shape = parent.AddShape(name, &assets.Cube(), &material, position, size);
	shape->solid = solid;
	return shape;
}

void SevenSegment(SceneNode& parent, Assets& assets, Material& material, int digit,
	float x, float y, float scale, float depth)
{
	static constexpr std::array<unsigned char, 10> mask{
		0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110,
		0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111
	};
	static constexpr std::array<glm::vec2, 7> centers{{
		{0.0f, 0.16f}, {0.11f, 0.08f}, {0.11f, -0.08f}, {0.0f, -0.16f},
		{-0.11f, -0.08f}, {-0.11f, 0.08f}, {0.0f, 0.0f}
	}};
	if (digit < 0 || digit > 9) return;
	for (int segment = 0; segment < 7; ++segment) {
		if ((mask[static_cast<size_t>(digit)] & (1u << segment)) == 0) continue;
		const bool vertical = segment == 1 || segment == 2 || segment == 4 || segment == 5;
		parent.AddShape("RaisedDigitSegment", &assets.Cube(), &material,
			{x + centers[static_cast<size_t>(segment)].x * scale,
			 y + centers[static_cast<size_t>(segment)].y * scale, depth},
			{(vertical ? 0.035f : 0.17f) * scale, (vertical ? 0.14f : 0.035f) * scale, 0.025f * scale});
	}
}

void AddTrainWheels(SceneNode& train, Assets& assets, Material& wheel, float z)
{
	for (float x : {-0.30f, 0.30f}) {
		SceneNode* shape = train.AddShape("TrainWheel", &assets.Cylinder(), &wheel,
			{x, 0.145f, z}, {0.12f, 0.12f, 0.12f}, {0, 0, 90});
		shape->solid = false;
	}
}

void BuildTrain(SceneNode& world, Assets& assets)
{
	SceneNode* train = world.AddChild("ClueToyTrain");
	train->local.position = TrainPosition;
	train->local.rotation.y = 90.0f; // Set the train beside the north wall, along the corridor rather than across it.
	Material& body = Wood(assets, "haunted-train-wood", {0.38f, 0.14f, 0.12f});
	Material& trim = assets.Mat("haunted-train-brass", {0.72f, 0.49f, 0.20f}, 0.72f, 72.0f);
	trim.reflectivity = 0.12f;
	Material& coal = assets.Mat("haunted-train-iron", {0.13f, 0.15f, 0.17f}, 0.62f, 48.0f);
	Box(*train, assets, "LocomotiveChassis", body, {0, 0.27f, -0.38f}, {0.62f, 0.30f, 0.82f}, true);
	train->AddShape("Boiler", &assets.Cylinder(), &body, {0, 0.48f, -0.48f}, {0.27f, 0.55f, 0.27f}, {90, 0, 0});
	Box(*train, assets, "Cab", body, {0, 0.59f, -0.05f}, {0.52f, 0.43f, 0.38f});
	Box(*train, assets, "CabWindow", trim, {0, 0.65f, 0.15f}, {0.32f, 0.17f, 0.025f});
	train->AddShape("Chimney", &assets.Cylinder(), &trim, {0, 0.76f, -0.73f}, {0.12f, 0.25f, 0.12f});
	train->AddShape("Headlamp", &assets.Sphere(), &trim, {0, 0.48f, -0.82f}, {0.13f, 0.13f, 0.10f});
	AddTrainWheels(*train, assets, coal, -0.62f);
	AddTrainWheels(*train, assets, coal, -0.15f);

	// The locomotive pulls exactly two distinct toy cars.
	for (int car = 0; car < 2; ++car) {
		const float z = 0.36f + static_cast<float>(car) * 0.65f;
		Box(*train, assets, car == 0 ? "FirstTrainCar" : "SecondTrainCar", body,
			{0, 0.20f, z}, {0.58f, 0.22f, 0.52f});
		Box(*train, assets, "CarLoad", trim, {0, 0.40f, z}, {0.39f, 0.20f, 0.34f});
		AddTrainWheels(*train, assets, coal, z - 0.17f);
		AddTrainWheels(*train, assets, coal, z + 0.17f);
	}
}

void BuildBlocks(SceneNode& world, Assets& assets)
{
	SceneNode* pile = world.AddChild("ClueColoredToyBlocks");
	pile->local.position = BlocksPosition;
	const std::array<glm::vec2, 7> digitSeven{{
		{-0.48f, -0.42f}, {-0.16f, -0.42f}, {0.16f, -0.42f}, {0.48f, -0.42f},
		{0.32f, -0.08f}, {0.00f, 0.20f}, {-0.32f, 0.48f}
	}};
	const std::array<glm::vec3, 3> colors{{{0.78f, 0.12f, 0.12f}, {0.92f, 0.67f, 0.14f}, {0.12f, 0.28f, 0.78f}}};
	std::array<Material*, 3> materials{};
	for (size_t i = 0; i < colors.size(); ++i) {
		Material& material = assets.Mat("clue-block-color-" + std::to_string(i), colors[i], 0.22f, 22.0f);
		material.texture = assets.SlotTexture(Assets::BlockSlot);
		material.rtTextureSlot = Assets::BlockSlot;
		materials[i] = &material;
	}
	for (size_t i = 0; i < digitSeven.size(); ++i) {
		const glm::vec2 p = digitSeven[i];
		Box(*pile, assets, "NumberSevenBlock", *materials[i % materials.size()],
			{p.x, 0.16f, p.y}, {0.30f, 0.32f, 0.30f});
	}
}

void BuildClock(SceneNode& world, Assets& assets)
{
	SceneNode* clock = world.AddChild("ClueOldWallClock");
	clock->local.position = ClockPosition;
	clock->local.rotation.x = 90.0f; // Cylinder cap faces +Z toward Penny in the hallway.
	Material& clockCase = assets.Mat("old-clock-blue-rim", {0.035f, 0.16f, 0.56f}, 0.35f, 38.0f);
	Material& face = assets.Mat("toy-story-clock-face", {1.0f, 1.0f, 1.0f}, 0.1f, 20.0f);
	face.texture = assets.SlotTexture(Assets::ClockSlot);
	face.rtTextureSlot = Assets::ClockSlot;
	face.ka = 1.0f;
	face.kd = 0.9f;
	face.ks = 0.1f;
	// The cylinder provides a real circular silhouette and its planar cap maps the provided square image 1:1.
	clock->AddShape("ClockBlueCircularCase", &assets.Cylinder(), &clockCase, {0, 0, 0}, {1.50f, 0.18f, 1.50f});
	clock->AddShape("ClockToyStoryPrintedFace", &assets.Cylinder(), &face, {0, 0.095f, 0}, {1.38f, 0.025f, 1.38f});
}

void BuildKeypad(SceneNode& world, Assets& assets)
{
	SceneNode* keypad = world.AddChild("ToyRoomCombinationKeypad");
	keypad->local.position = KeypadPosition;
	keypad->local.rotation.y = 180.0f; // Buttons face Penny in the hallway.
	Material& wood = Wood(assets, "combination-keypad-case", {0.24f, 0.17f, 0.12f});
	Material& metal = assets.Mat("combination-keypad-metal", {0.34f, 0.37f, 0.40f}, 0.78f, 88.0f);
	Material& digitInk = assets.Mat("combination-keypad-digit", {0.08f, 0.09f, 0.10f}, 0.25f, 25.0f);
	Box(*keypad, assets, "KeypadSolidHousing", wood, {0, 0, 0}, {1.18f, 1.58f, 0.18f}, true);
	keypad->AddShape("KeypadSteelPlate", &assets.Cube(), &metal, {0, 0, 0.105f}, {1.02f, 1.40f, 0.045f});
	for (int digit = 0; digit <= 9; ++digit) {
		const int row = digit / 5;
		const int column = digit % 5;
		const float x = (static_cast<float>(column) - 2.0f) * 0.19f;
		const float y = row == 0 ? 0.36f : 0.03f;
		keypad->AddShape("KeypadButton", &assets.Cube(), &metal, {x, y, 0.15f}, {0.155f, 0.22f, 0.055f});
		SevenSegment(*keypad, assets, digitInk, digit, x, y, 0.42f, 0.184f);
	}
	keypad->AddShape("KeypadEnterButton", &assets.Cylinder(), &metal, {0.39f, -0.48f, 0.15f},
		{0.15f, 0.055f, 0.15f}, {90, 0, 0});
}
}

void HallwayPuzzle::Build(SceneNode& world, Assets& assets, const HouseRig& house)
{
	leftDoor = house.roomDoorLeft;
	rightDoor = house.roomDoorRight;
	BuildTrain(world, assets);
	BuildBlocks(world, assets);
	BuildClock(world, assets);
	BuildKeypad(world, assets);
	for (SceneNode* door : {leftDoor, rightDoor}) {
		if (!door) continue;
		if (SceneNode* leaf = door->Find("DoorLeaf")) leaf->solid = true;
	}
	code.Reset();
	keypadActive = false;
	unlocked = false;
	active = false;
}

void HallwayPuzzle::Begin()
{
	Reset();
	active = true;
	if (leftDoor) leftDoor->local.rotation.y = 0.0f;
	if (rightDoor) rightDoor->local.rotation.y = 0.0f;
}

void HallwayPuzzle::Reset()
{
	code.Reset();
	keypadActive = false;
	unlocked = false;
	active = false;
	status.clear();
	doorOpen = 0.0f;
}

void HallwayPuzzle::Update(float dt)
{
	if (!active) return;
	const float target = unlocked ? 90.0f : 0.0f;
	doorOpen += (target - doorOpen) * (1.0f - std::exp(-dt * 6.0f));
	if (leftDoor) leftDoor->local.rotation.y = doorOpen;
	if (rightDoor) rightDoor->local.rotation.y = -doorOpen;
}

std::string HallwayPuzzle::Interact(const glm::vec3& pennyPosition)
{
	if (!active || keypadActive || unlocked) return {};
	struct NearbyClue { PuzzleCode::Clue clue; glm::vec3 position; const char* found; const char* repeated; };
	static constexpr NearbyClue clues[]{
		{PuzzleCode::Clue::Train, TrainPosition, "Toy Train: A faded number 2 is written underneath.", "Toy Train: 2"},
		{PuzzleCode::Clue::Blocks, BlocksPosition, "Colored Toy Blocks: A faded number 7 is printed underneath.", "Toy Blocks: 7"},
		{PuzzleCode::Clue::Clock, ClockPosition, "Old Wall Clock: A faded number 5 is marked on the clock.", "Old Wall Clock: 5"}
	};
	const NearbyClue* closest = nullptr;
	float closestDistance = 2.25f;
	for (const NearbyClue& clue : clues) {
		const glm::vec2 delta = glm::vec2(pennyPosition.x - clue.position.x, pennyPosition.z - clue.position.z);
		const float distance = glm::length(delta);
		if (distance < closestDistance) { closestDistance = distance; closest = &clue; }
	}
	if (closest) {
		const bool alreadyFound = code.Found(closest->clue);
		code.Discover(closest->clue);
		status = alreadyFound ? closest->repeated : closest->found;
		return status;
	}
	const glm::vec2 keypadDelta(pennyPosition.x - KeypadPosition.x, pennyPosition.z - KeypadPosition.z);
	if (glm::length(keypadDelta) <= 1.75f) {
		keypadActive = true;
		status = code.AllCluesFound()
			? "Keypad active. Type three digits, then press Enter."
			: "Inspect all three clues before entering the code.";
		return status;
	}
	status = "Move closer to a clue or the keypad.";
	return status;
}

std::string HallwayPuzzle::InteractionHint(const glm::vec3& pennyPosition) const
{
	if (!active || unlocked || keypadActive) return {};
	struct Target { PuzzleCode::Clue clue; glm::vec3 position; const char* name; };
	static constexpr Target clues[]{
		{PuzzleCode::Clue::Train, TrainPosition, "Toy Train"},
		{PuzzleCode::Clue::Clock, ClockPosition, "Wall Clock"},
		{PuzzleCode::Clue::Blocks, BlocksPosition, "Colored Toy Blocks"}
	};
	const Target* closest = nullptr;
	float closestDistance = 2.25f;
	for (const Target& clue : clues) {
		const glm::vec2 delta(pennyPosition.x - clue.position.x, pennyPosition.z - clue.position.z);
		const float distance = glm::length(delta);
		if (distance < closestDistance) { closestDistance = distance; closest = &clue; }
	}
	if (closest) {
		return std::string("ENTER - ") + (code.Found(closest->clue) ? "Review " : "Inspect ") + closest->name;
	}
	const glm::vec2 keypadDelta(pennyPosition.x - KeypadPosition.x, pennyPosition.z - KeypadPosition.z);
	if (glm::length(keypadDelta) <= 1.75f) return "ENTER - Use keypad";
	return {};
}

void HallwayPuzzle::CancelKeypad()
{
	if (!keypadActive) return;
	keypadActive = false;
	status = "Keypad closed.";
}

PuzzleCode::Result HallwayPuzzle::Submit()
{
	if (!keypadActive) return PuzzleCode::Result::Incomplete;
	const PuzzleCode::Result result = code.Submit();
	switch (result) {
	case PuzzleCode::Result::Incomplete:
		status = "Enter three digits.";
		break;
	case PuzzleCode::Result::MissingClues:
		status = "Inspect all three clues first.";
		break;
	case PuzzleCode::Result::Incorrect:
		status = "Incorrect Code";
		break;
	case PuzzleCode::Result::Correct:
		unlocked = true;
		keypadActive = false;
		status = "Correct";
		break;
	}
	return result;
}
