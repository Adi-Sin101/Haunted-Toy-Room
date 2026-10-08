#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "geometry/Primitives.h"
#include "scene/SceneNode.h"
#include "world/PhysicsWorld.h"
#include "world/Room.h"
#include "world/PuzzleCode.h"

namespace {
int checks = 0;
void Require(bool condition, const char* message)
{
	if (!condition) throw std::runtime_error(message);
	++checks; std::cout << "PASS " << message << '\n';
}
void Run()
{
	PuzzleCode puzzle;
	Require(puzzle.Display() == "CODE: _ _ _", "three-digit puzzle starts with three blank slots");
	Require(!puzzle.Found(PuzzleCode::Clue::Train), "clue starts undiscovered until inspection");
	puzzle.Discover(PuzzleCode::Clue::Train);
	puzzle.Discover(PuzzleCode::Clue::Train);
	Require(puzzle.Found(PuzzleCode::Clue::Train) && puzzle.CluesFound() == 1, "repeated clue inspection counts only once");
	puzzle.Reset();
	Require(puzzle.AddDigit(2) && puzzle.Display() == "CODE: 2 _ _", "keypad entry updates the first code slot");
	Require(puzzle.AddDigit(5) && puzzle.Display() == "CODE: 2 5 _", "keypad entry updates the second code slot");
	Require(puzzle.Backspace() && puzzle.Display() == "CODE: 2 _ _", "keypad backspace removes the last digit");
	Require(puzzle.AddDigit(5) && puzzle.AddDigit(7), "keypad accepts the remaining code digits");
	Require(puzzle.Submit() == PuzzleCode::Result::MissingClues && !puzzle.Complete(), "correct code cannot bypass undiscovered clues");
	Require(puzzle.Display() == "CODE: _ _ _", "missing-clue attempt clears the entered digits");
	puzzle.Discover(PuzzleCode::Clue::Train);
	puzzle.Discover(PuzzleCode::Clue::Clock);
	puzzle.Discover(PuzzleCode::Clue::Blocks);
	Require(puzzle.AllCluesFound() && puzzle.CluesFound() == 3, "all three physical clues can be recorded");
	for (int digit : {2, 5, 8}) puzzle.AddDigit(digit);
	Require(puzzle.Submit() == PuzzleCode::Result::Incorrect && !puzzle.Complete(), "wrong code remains locked");
	Require(puzzle.Display() == "CODE: _ _ _", "wrong code clears without resetting clue progress");
	for (int digit : {2, 5, 7}) puzzle.AddDigit(digit);
	Require(puzzle.Submit() == PuzzleCode::Result::Correct && puzzle.Complete(), "257 unlocks after all clues are found");
	Require(!puzzle.AddDigit(9) && puzzle.Display() == "CODE: 2 5 7", "completed code cannot be changed");

	SceneNode hallway("Hallway");
	Mesh hallwayDoorMesh("HallwayDoorMesh", PrimitiveType::Cube, Primitives::Cube());
	SceneNode* closedDoor = hallway.AddShape("ClosedHallDoor", &hallwayDoorMesh, nullptr,
		{10.05f, 2.35f, 4.0f}, {0.08f, 4.7f, 3.0f});
	closedDoor->solid = true;
	SceneNode* hallwayActor = hallway.AddChild("HallwayActor");
	hallwayActor->local.position = {15.0f, 0.0f, 4.4f};
	hallway.UpdateWorld(glm::mat4(1.0f));
	PhysicsWorld hallwayPhysics;
	hallwayPhysics.Init(hallway, {});
	hallwayPhysics.EnableHallway(true);
	hallwayPhysics.AddActor(hallwayActor, {0.34f, 0.48f, 0.42f}, {0, 0.48f, 0});
	hallwayActor->local.position = {7.0f, 0.0f, 4.4f};
	hallwayPhysics.ConstrainActor(hallwayActor, {15.0f, 0.0f, 4.4f});
	Require(hallwayActor->local.position.x > 10.3f, "closed hallway door blocks Penny from returning to the Toy Room");

	SceneNode boundedHallway("BoundedHallway");
	Mesh hallWallMesh("HallWallMesh", PrimitiveType::Cube, Primitives::Cube());
	SceneNode* upperHallWall = boundedHallway.AddShape("HallSide", &hallWallMesh, nullptr,
		{13.5f, 2.4f, RoomSize::DoorHigh}, {RoomSize::HallEnd - RoomSize::HalfWidth, 4.8f, 0.16f});
	upperHallWall->solid = true;
	SceneNode* leftHinge = nullptr;
	SceneNode* rightHinge = nullptr;
	for (int side = 0; side < 2; ++side) {
		SceneNode* hinge = boundedHallway.AddChild(side == 0 ? "RoomDoorLeft" : "RoomDoorRight");
		(side == 0 ? leftHinge : rightHinge) = hinge;
		hinge->local.position = {RoomSize::HalfWidth + 0.05f, 0.0f, side == 0 ? RoomSize::DoorLow : RoomSize::DoorHigh};
		const float sign = side == 0 ? 1.0f : -1.0f;
		SceneNode* leaf = hinge->AddShape("DoorLeaf", &hallWallMesh, nullptr,
			{0.0f, 2.35f, sign * 1.5f}, {0.08f, 4.7f, 2.96f});
		leaf->solid = true;
	}
	SceneNode* boundedActor = boundedHallway.AddChild("BoundedActor");
	boundedActor->local.position = {15.0f, 0.0f, 4.4f};
	boundedHallway.UpdateWorld(glm::mat4(1.0f));
	PhysicsWorld boundsPhysics;
	boundsPhysics.Init(boundedHallway, {});
	boundsPhysics.EnableHallway(true);
	boundsPhysics.AddActor(boundedActor, {0.34f, 0.48f, 0.42f}, {0, 0.48f, 0});
	boundedActor->local.position = {4.0f, 0.0f, 8.5f};
	boundsPhysics.ConstrainActor(boundedActor, {15.0f, 0.0f, 4.4f});
	Require(boundedActor->local.position.z <= RoomSize::DoorHigh && boundedActor->local.position.x > 10.3f,
		"hallway side wall blocks actors from leaving through its end");
	leftHinge->local.rotation.y = 90.0f;
	rightHinge->local.rotation.y = -90.0f;
	boundedHallway.UpdateWorld(glm::mat4(1.0f));
	boundsPhysics.Update(1.0f / 60.0f);
	boundedActor->local.position = {8.0f, 0.0f, 4.4f};
	boundsPhysics.ConstrainActor(boundedActor, {10.5f, 0.0f, 4.4f});
	Require(boundedActor->local.position.x < 9.5f, "open Toy Room door lets Penny walk into the room");

	Mesh cube("CheckCube", PrimitiveType::Cube, Primitives::Cube());
	SceneNode scene("CheckScene");
	SceneNode* wall = scene.AddShape("Wall", &cube, nullptr, {0, 1, 0}, {1, 2, 1}); wall->solid = true;
	SceneNode* lower = scene.AddShape("Lower", &cube, nullptr, {3, 0.3f, 1}, glm::vec3(0.6f));
	SceneNode* upper = scene.AddShape("Upper", &cube, nullptr, {3, 0.9f, 1}, glm::vec3(0.6f));
	SceneNode* actor = scene.AddChild("Actor"); actor->local.position = {-3, 0, 0};
	SceneNode* other = scene.AddChild("Other"); other->local.position = {-2, 0, 2};
	scene.UpdateWorld(glm::mat4(1));
	PhysicsWorld world; world.Init(scene, {lower, upper});
	world.AddActor(actor, {0.35f, 1, 0.35f}, {0, 1, 0});
	world.AddActor(other, {0.35f, 1, 0.35f}, {0, 1, 0});
	actor->local.position = {3, 0, 0}; world.ConstrainActor(actor, {-3, 0, 0});
	Require(actor->local.position.x < -0.85f, "fast actor movement cannot tunnel through furniture");
	glm::vec3 camera = world.MoveCamera({-3, 1, 0}, {3, 1, 0});
	Require(camera.x < -0.70f, "camera sweep cannot tunnel through furniture");
	camera = world.MoveCamera({-3, 1, 0}, {3, 1, 0.4f});
	Require(camera.x < -0.70f && camera.z > 0.35f, "camera slides along a contacted face");
	camera = world.MoveCamera({0, 1, 0}, {0, 1, 0});
	Require(std::abs(camera.x) >= 0.70f || std::abs(camera.z) >= 0.70f || camera.y > 2.20f, "camera recovers from an overlapping edited object");
	camera = world.MoveCamera({1, 4, 4}, {100, 100, -100});
	Require(std::abs(camera.x) < RoomSize::HalfWidth && std::abs(camera.z) < RoomSize::HalfDepth
		&& camera.y > 0.2f && camera.y < RoomSize::Height, "camera stays inside all six room boundaries");
	actor->local.position = {0, 0, 2}; world.ConstrainActor(actor, {-4, 0, 2});
	Require(actor->local.position.x < -2.70f, "characters remain separated from other characters");
	actor->local.position = {-4, 0, 0}; actor->local.scale = glm::vec3(100);
	world.ConstrainActor(actor, {-4, 0, 0});
	Require(actor->local.scale.x < 4.0f, "oversized edited actors fit the room");
	world.EnableActor(actor, false); world.EnableActor(other, false);
	wall->local.scale.x = 0.02f; scene.UpdateWorld(glm::mat4(1));
	camera = world.MoveCamera({-8, 1, 0}, {8, 1, 0});
	Require(camera.x < -0.20f, "thin barriers stop very large camera steps");
	wall->local.scale.x = 1.0f; scene.UpdateWorld(glm::mat4(1));
	for (int step = 0; step < 360; ++step) world.Update(1.0f / 120.0f);
	Require(glm::distance(upper->local.position, glm::vec3(3, 0.9f, 1)) < 0.04f, "a resting two-block stack remains stable");
	const float hit = world.FireLaser({3, 0.90f, -3}, {0, 0, 1}, 1.0f / 60.0f, nullptr);
	Require(hit > 3.6f && hit < 3.8f && world.Bodies()[1].velocity.z > 3.0f, "laser ray transfers an impulse to the nearest block");
	Require(glm::length(world.Bodies()[1].angular) > 10.0f, "laser impact starts block tumbling");
	for (int step = 0; step < 720; ++step) world.Update(1.0f / 120.0f);
	scene.UpdateWorld(glm::mat4(1));
	Require(upper->local.position.z > 1.5f, "struck block moves away from its stack");
	for (const auto& body : world.Bodies()) {
		const auto b = PhysicsWorld::ShapeBounds(body.node);
		Require(b.low.y >= -0.002f && b.high.y <= RoomSize::Height, "gravity preserves floor and ceiling clearance");
		Require(b.low.x > -RoomSize::HalfWidth && b.high.x < RoomSize::HalfWidth
			&& b.low.z > -RoomSize::HalfDepth && b.high.z < RoomSize::HalfDepth, "moving blocks stay within the room");
	}
	world.ResetBlocks();
	Require(glm::distance(upper->local.position, glm::vec3(3, 0.9f, 1)) < 0.001f
		&& glm::length(world.Bodies()[1].velocity) == 0, "rebuild restores the complete stack and clears velocity");
	lower->local.position = {0, 0.3f, 3}; upper->local.position = {0, 0.9f, 3};
	const float blocked = world.FireLaser({0, 0.9f, -3}, {0, 0, 1}, 0.2f, nullptr);
	Require(blocked < 2.6f && glm::length(world.Bodies()[1].velocity) == 0, "furniture blocks the laser before a hidden block");
	const glm::vec3 view = world.CameraSightline({-3, 1, 0}, {3, 1, 0}, nullptr);
	Require(view.x < -0.7f, "orbit and follow sightlines shorten before an obstruction");
}
}
int main()
{
	if (!glfwInit()) return 1;
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(64, 64, "Physics checks", nullptr, nullptr);
	if (!window) { glfwTerminate(); return 1; }
	glfwMakeContextCurrent(window);
	if (!gladLoadGL()) { glfwDestroyWindow(window); glfwTerminate(); return 1; }
	int result = 0;
	try { Run(); std::cout << checks << " physics checks passed.\n"; }
	catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; result = 1; }
	glfwDestroyWindow(window); glfwTerminate(); return result;
}
