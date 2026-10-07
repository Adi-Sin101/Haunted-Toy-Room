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
#include "math/Ray.h"
#include "scene/Transform.h"

namespace {
int checks = 0;
void Require(bool condition, const char* message)
{
	if (!condition) throw std::runtime_error(message);
	++checks; std::cout << "PASS " << message << '\n';
}
void Run()
{
    const glm::mat4 sheared=t3d::shear(2.0f,0,0,0,0,0)*t3d::scale({1,2,1});
    const float radius=t3d::unitBoundsRadius(sheared);
    bool contains=true;
    for (float x:{-0.5f,0.5f}) for (float y:{-0.5f,0.5f}) for (float z:{-0.5f,0.5f})
        contains=contains && glm::length(glm::vec3(sheared*glm::vec4(x,y,z,0)))<=radius+1e-5f;
    Require(contains,"sheared culling sphere contains every transformed corner");
    const glm::vec3 tangent(sheared*glm::vec4(1,0,0,0));
    const glm::vec3 normal=t3d::normalMatrix(sheared)*glm::vec3(0,1,0);
    Require(std::abs(glm::dot(normal,tangent))<1e-5f,"inverse transpose preserves normal perpendicularity under shear");
    const Ray forward{{0,0,-2},{0,0,1}};
    Require(std::abs(RayIntersect::Sphere(forward).t-1.5f)<1e-5f,"analytic sphere nearest hit");
    Require(std::abs(RayIntersect::Cube(forward).t-1.5f)<1e-5f,"analytic cube nearest hit");
    Require(std::abs(RayIntersect::Cylinder(forward).t-1.5f)<1e-5f,"analytic cylinder side hit");
    Require(std::abs(RayIntersect::Cone(forward).t-1.75f)<1e-5f,"analytic cone side hit");
    Require(std::abs(RayIntersect::Plane({{0,2,0},{0,-1,0}}).t-2)<1e-5f,"one-sided plane front hit");
    Require(RayIntersect::Plane({{0,-2,0},{0,1,0}}).t<0,"one-sided plane back miss");
    Require(std::abs(RayIntersect::Cube({{0,0,0},{1,0,0}}).t-0.5f)<1e-5f,"ray originating inside a cube finds exit");
    Require(std::abs(RayIntersect::Cylinder({{0,2,0},{0,-1,0}}).t-1.5f)<1e-5f,"axial cylinder ray hits cap");
    Require(std::abs(RayIntersect::Cone({{-0.5f,0,0},{0.5f,-1,0}}).t-0.25f)<1e-5f,"cone generatrix parallel ray solves linear equation");
    const glm::mat4 model=t3d::translate({0,0,3})*t3d::scale({2,1,4});
    Require(std::abs(RayIntersect::Object(PrimitiveType::Sphere,glm::inverse(model),forward).t-3)<1e-5f,"scaled object rays preserve world parameter");
    Transform transform; transform.position={2,3,4}; transform.rotation={32,48,11}; transform.scale={2,0.7f,1.3f}; transform.basis=sheared;
    const glm::mat4 expected=t3d::translate(transform.position)*t3d::rotateY(glm::radians(48.0f))*t3d::rotateX(glm::radians(32.0f))*t3d::rotateZ(glm::radians(11.0f))*sheared*t3d::scale(transform.scale);
    bool same=true; for(int c=0;c<4;++c) for(int r=0;r<4;++r) same=same && std::abs(expected[c][r]-transform.Matrix()[c][r])<1e-5f;
    Require(same,"optimised transform equals TRS product with shear");

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
	world.SetIndependentActor(other);
	actor->local.position={0,0,2}; world.ConstrainActor(actor,{-4,0,2});
	Require(actor->local.position.x>-0.1f,"live-owned actor cannot obstruct another actor's route");
	world.SetIndependentActor(nullptr);
	actor->local.position={0,0,2}; world.ConstrainActor(actor,{-4,0,2});
	Require(actor->local.position.x<-2.70f,"releasing live ownership restores actor collision");
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
	world.EnableHallway(true);
	const auto hallwayCamera=world.MoveCamera({9.5f,2,4},{14,2,4});
	Require(hallwayCamera.x>13.9f && hallwayCamera.x<RoomSize::HallEnd,"camera can inspect toys in the connected hallway");
	const auto sealedCamera=world.MoveCamera({9.5f,2,-4},{14,2,-4});
	Require(sealedCamera.x<RoomSize::HalfWidth,"hallway access preserves the closed wall boundaries");
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
