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

namespace {
int checks = 0;
void Require(bool condition, const char* message)
{
	if (!condition) throw std::runtime_error(message);
	++checks; std::cout << "PASS " << message << '\n';
}
void Run()
{
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
