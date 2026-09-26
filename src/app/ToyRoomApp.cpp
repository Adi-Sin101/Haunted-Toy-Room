#include "ToyRoomApp.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "characters/Bullseye.h"
#include "characters/Humanoid.h"
#include "characters/RCCar.h"
#include "math/Ray.h"
#include "render/BmpLoader.h"
#include "world/Room.h"

namespace {

// Fixed slots in the light list.
enum LightSlot : int { SkyLight = 0, LampBulb, LampSpot, HeadlightL, HeadlightR, LaserGlow, GhostGlow, LightSlotCount };

const char* EditOpName(EditOp op)
{
	switch (op) {
	case EditOp::Translate: return "Translate";
	case EditOp::Rotate: return "Rotate";
	case EditOp::Scale: return "Scale";
	case EditOp::Shear: return "Shear";
	}
	return "?";
}

const char* CameraModeName(CameraMode m)
{
	switch (m) {
	case CameraMode::Free: return "Free";
	case CameraMode::Orbit: return "Orbit";
	case CameraMode::Follow: return "Follow";
	}
	return "?";
}

// World-space direction of a node's local -Y axis (lamp head, headlights point along it / +Z).
glm::vec3 AxisOf(const SceneNode* node, int axis, float sign)
{
	return glm::normalize(glm::vec3(node->World()[axis]) * sign);
}

} // namespace

ToyRoomApp::ToyRoomApp(const LaunchOptions& options) : Application(1600, 900, "Haunted Toy Room"), launch(options)
{
	assets.Load();
	renderer.Init(assets);
	rayTracer.Init(assets);
	BuildScene();
	camera.Reset();
	PrintHelp();
	ApplyLaunchOptions();
}

ToyRoomApp::~ToyRoomApp() = default;

// =============================================================================================
// Scene construction
// =============================================================================================
void ToyRoomApp::BuildScene()
{
	scene = std::make_unique<SceneNode>("World");
	const RoomRig rig = BuildRoom(*scene, assets);
	environment.Init(rig);

	auto woodyPtr = std::make_unique<Humanoid>(*scene, assets, WoodyStyle(), glm::vec3(-3.0f, 0.0f, 0.5f), 20.0f);
	auto jessiePtr = std::make_unique<Humanoid>(*scene, assets, JessieStyle(), glm::vec3(-0.8f, 0.0f, 2.0f), 0.0f);
	auto bullseyePtr = std::make_unique<Bullseye>(*scene, assets, glm::vec3(2.2f, 0.0f, 0.3f), -30.0f);
	auto buzzPtr = std::make_unique<Buzz>(*scene, assets, glm::vec3(0.4f, 0.0f, -1.8f), 10.0f);
	auto carPtr = std::make_unique<RCCar>(*scene, assets, glm::vec3(4.5f, 0.0f, 3.0f), -90.0f);
	woody = woodyPtr.get();
	jessie = jessiePtr.get();
	bullseye = bullseyePtr.get();
	buzz = buzzPtr.get();
	car = carPtr.get();

	AddSelectable("Woody", woody->Root(), woody, 1, 1.1f, 4.0f);
	AddSelectable("Jessie", jessie->Root(), jessie, 2, 1.1f, 4.0f);
	AddSelectable("Bullseye", bullseye->Root(), bullseye, 3, 1.2f, 5.0f);
	AddSelectable("Buzz", buzz->Root(), buzz, 4, 1.1f, 4.0f);
	AddSelectable("RC Car", car->Root(), car, 5, 0.4f, 3.5f);
	ballId = static_cast<int>(selectables.size());
	AddSelectable("Ball", rig.ball, nullptr, 6, 0.0f, 3.0f);
	lampId = static_cast<int>(selectables.size());
	AddSelectable("Desk Lamp", rig.lamp, nullptr, 7, 1.0f, 4.0f);
	ghostId = static_cast<int>(selectables.size());
	AddSelectable("Ghost", rig.ghost, nullptr, 8, 0.3f, 5.0f);

	characters.push_back(std::move(woodyPtr));
	characters.push_back(std::move(jessiePtr));
	characters.push_back(std::move(bullseyePtr));
	characters.push_back(std::move(buzzPtr));
	characters.push_back(std::move(carPtr));

	// Night-time patrol loops (inside the free floor area)
	story.Add(woody, { { -3.5f, 0, 2.5f }, { -1.0f, 0, 4.0f }, { 1.5f, 0, 2.5f }, { -1.5f, 0, 0.0f } });
	story.Add(jessie, { { 0.5f, 0, 3.5f }, { 3.0f, 0, 4.0f }, { 1.0f, 0, 1.0f }, { -2.0f, 0, 3.0f } });
	story.Add(bullseye, { { 4.5f, 0, -1.5f }, { 5.0f, 0, 1.2f }, { 2.0f, 0, 3.8f }, { -0.5f, 0, 0.5f }, { 1.5f, 0, -2.5f } });
	story.Add(buzz, { { 3.0f, 0, -3.0f }, { -2.0f, 0, -3.0f }, { -4.0f, 0, 1.0f }, { 0.5f, 0, 0.5f } }, true);
	story.Add(car, { { 3.8f, 0, 4.8f }, { -5.5f, 0, 4.8f }, { -5.5f, 0, -1.5f }, { 4.0f, 0, -2.5f } }); // loop stays clear of the toy blocks

	// Light list (fixed slots, positions/directions refreshed every frame from the scene nodes)
	lights.resize(LightSlotCount);
	lights[SkyLight] = { "Sun/Moon", LightType::Directional, true, true };
	lights[LampBulb] = { "Lamp bulb", LightType::Point, true, true };
	lights[LampBulb].color = { 1.0f, 0.82f, 0.55f };
	lights[LampBulb].linear = 0.14f;
	lights[LampBulb].quadratic = 0.07f;
	lights[LampSpot] = { "Lamp spot", LightType::Spot, true, true };
	lights[LampSpot].color = { 1.0f, 0.88f, 0.65f };
	lights[LampSpot].linear = 0.05f;
	lights[LampSpot].quadratic = 0.01f;
	lights[LampSpot].innerCutoff = std::cos(glm::radians(22.0f));
	lights[LampSpot].outerCutoff = std::cos(glm::radians(34.0f));
	for (int i : { HeadlightL, HeadlightR }) {
		lights[static_cast<size_t>(i)] = { i == HeadlightL ? "Headlight L" : "Headlight R", LightType::Spot, true, false };
		lights[static_cast<size_t>(i)].color = { 1.0f, 0.95f, 0.8f };
		lights[static_cast<size_t>(i)].linear = 0.1f;
		lights[static_cast<size_t>(i)].quadratic = 0.05f;
		lights[static_cast<size_t>(i)].innerCutoff = std::cos(glm::radians(12.0f));
		lights[static_cast<size_t>(i)].outerCutoff = std::cos(glm::radians(22.0f));
	}
	lights[LaserGlow] = { "Laser glow", LightType::Point, false, false };
	lights[LaserGlow].color = { 1.0f, 0.1f, 0.1f };
	lights[LaserGlow].linear = 0.35f;
	lights[LaserGlow].quadratic = 0.4f;
	lights[GhostGlow] = { "Ghost glow", LightType::Point, true, false };
	lights[GhostGlow].color = { 0.5f, 0.7f, 1.0f };
	lights[GhostGlow].linear = 0.3f;
	lights[GhostGlow].quadratic = 0.15f;

	scene->UpdateWorld(glm::mat4(1.0f));
}

void ToyRoomApp::AddSelectable(const std::string& name, SceneNode* node, Character* character, int key, float focusHeight, float focusDistance)
{
	const int id = static_cast<int>(selectables.size());
	node->SetOwner(id);
	selectables.push_back({ name, node, character, key, focusHeight, focusDistance, node->local });
}

// =============================================================================================
// Update
// =============================================================================================
void ToyRoomApp::OnUpdate(float dt)
{
	HandleGlobalKeys();
	HandleSelection();
	HandleCamera(dt);
	if (editMode)
		HandleEditMode(dt);
	else
		HandleObjectControl(dt);

	const Selectable* sel = Selected();
	story.enabled = environment.hauntingEnabled;
	environment.Update(dt, static_cast<float>(time), selectedId == lampId, selectedId == ballId, selectedId == ghostId);
	const Character* selectedCharacter = sel ? sel->character : nullptr;
	if (jessieMounted && selectedCharacter == jessie)
		selectedCharacter = bullseye;
	story.Update(dt, environment.IsNight(), selectedCharacter, jessieMounted ? jessie : nullptr);

	for (auto& c : characters)
		c->Animate(dt, static_cast<float>(time));

	// One depth-first pass computes every world matrix: world = parent.world * local.
	scene->UpdateWorld(glm::mat4(1.0f));
	UpdateLights();

	// Console feedback only when the driven character starts / stops.
	if (Character* driven = DrivenCharacter()) {
		const bool moving = driven->IsMoving();
		if (moving != wasMoving)
			std::cout << driven->Name() << (moving ? " moving" : " stopped") << "\n";
		wasMoving = moving;
	}

	titleTimer -= dt;
	if (titleTimer <= 0.0f) {
		UpdateTitle();
		titleTimer = 0.25f;
	}
}

void ToyRoomApp::HandleGlobalKeys()
{
	auto flip = [](bool& b, const char* label) {
		b = !b;
		std::cout << label << (b ? ": ON" : ": OFF") << "\n";
	};

	if (input.Pressed(GLFW_KEY_ESCAPE)) Close();
	if (input.Pressed(GLFW_KEY_H)) PrintHelp();

	if (input.Pressed(GLFW_KEY_F1)) flip(settings.wireframe, "Wireframe");
	if (input.Pressed(GLFW_KEY_F2)) {
		settings.shading = static_cast<ShadingMode>((static_cast<int>(settings.shading) + 1) % 4);
		std::cout << "Shading: " << ToString(settings.shading) << "\n";
	}
	if (input.Pressed(GLFW_KEY_F3)) flip(settings.textures, "Textures");
	if (input.Pressed(GLFW_KEY_F4)) {
		flip(settings.rayTracing, "Ray tracing");
		if (settings.rayTracing)
			std::cout << "  (resolution x" << settings.rayScale << ", bounces " << settings.rayBounces << "; -/= resolution, 9 bounces)\n";
	}
	if (input.Pressed(GLFW_KEY_F5)) flip(settings.ambient, "Ambient");
	if (input.Pressed(GLFW_KEY_F6)) flip(settings.diffuse, "Diffuse");
	if (input.Pressed(GLFW_KEY_F7)) flip(settings.specular, "Specular");
	if (input.Pressed(GLFW_KEY_F8)) {
		lights[SkyLight].enabled = !lights[SkyLight].enabled;
		std::cout << "Sun/Moon light: " << (lights[SkyLight].enabled ? "ON" : "OFF") << "\n";
	}
	if (input.Pressed(GLFW_KEY_F9)) flip(settings.showNormals, "Normals of selected object");
	if (input.Pressed(GLFW_KEY_F10)) flip(settings.showVertices, "Vertices of selected object");
	if (input.Pressed(GLFW_KEY_F11)) flip(settings.showGizmo, "Local axes gizmo");
	if (input.Pressed(GLFW_KEY_F12)) SaveScreenshot();
	if (input.Pressed(GLFW_KEY_V)) DumpSelectedGeometry(input.ShiftDown());

	if (input.Pressed(GLFW_KEY_MINUS)) settings.rayScale = std::max(0.2f, settings.rayScale - 0.1f);
	if (input.Pressed(GLFW_KEY_EQUAL)) settings.rayScale = std::min(1.0f, settings.rayScale + 0.1f);
	if (input.Pressed(GLFW_KEY_9)) {
		settings.rayBounces = (settings.rayBounces + 1) % 5;
		std::cout << "Ray bounces: " << settings.rayBounces << "\n";
	}

	// World clock and story
	if (input.Pressed(GLFW_KEY_P)) flip(environment.paused, "World clock paused");
	if (input.Pressed(GLFW_KEY_N)) flip(environment.hauntingEnabled, "Story / haunting");
	if (input.Pressed(GLFW_KEY_LEFT_BRACKET)) {
		environment.timeScale = std::max(0.25f, environment.timeScale * 0.5f);
		std::cout << "Time speed x" << environment.timeScale << "\n";
	}
	if (input.Pressed(GLFW_KEY_RIGHT_BRACKET)) {
		environment.timeScale = std::min(32.0f, environment.timeScale * 2.0f);
		std::cout << "Time speed x" << environment.timeScale << "\n";
	}
	if (selectedId != lampId) { // with the lamp selected , and . change its brightness instead
		if (input.Down(GLFW_KEY_COMMA)) environment.Scrub(-0.05f);
		if (input.Down(GLFW_KEY_PERIOD)) environment.Scrub(0.05f);
	}

	if (input.Pressed(GLFW_KEY_TAB)) {
		editMode = !editMode;
		std::cout << "Edit mode " << (editMode ? "ON" : "OFF");
		if (editMode) std::cout << " (" << EditOpName(editOp) << ") - T operation, J/L X, U/O Y, I/K Z, M mirror, Backspace reset";
		std::cout << "\n";
	}
}

void ToyRoomApp::HandleSelection()
{
	if (input.Pressed(GLFW_KEY_0) && !input.CtrlDown())
		Select(-1);
	for (int i = 0; i < static_cast<int>(selectables.size()); ++i) {
		const int key = selectables[static_cast<size_t>(i)].key;
		if (key > 0 && !input.CtrlDown() && input.Pressed(GLFW_KEY_0 + key))
			Select(i);
	}

	// Mouse picking: a left click (not a drag) casts a ray into the scene.
	if (input.MousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
		mouseDownPos = input.MousePosition();
		mouseDragDistance = 0.0f;
	}
	if (input.MouseDown(GLFW_MOUSE_BUTTON_LEFT))
		mouseDragDistance += glm::length(input.MouseDelta());
	if (input.MouseReleased(GLFW_MOUSE_BUTTON_LEFT) && mouseDragDistance < 4.0f) {
		const int id = Pick(input.MousePosition());
		if (id >= 0)
			Select(id);
	}
}

void ToyRoomApp::Select(int id)
{
	if (id == selectedId)
		return;
	if (Character* c = DrivenCharacter())
		c->Stop();
	selectedId = id;
	wasMoving = false;
	if (const Selectable* s = Selected()) {
		std::cout << "Selected: " << s->name;
		if (s->character == jessie && jessieMounted)
			std::cout << " (riding Bullseye - W/S/A/D drive Bullseye, R dismount)";
		else if (s->character)
			std::cout << "  [" << s->character->ControlsHelp() << (s->character == jessie || s->character == bullseye ? "  R mount/dismount" : "") << "]";
		else if (id == lampId)
			std::cout << "  [A/D swivel  W/S tilt  R power  ,/. brightness]";
		else if (id == ballId)
			std::cout << "  [W/S/A/D push (camera relative)  SPACE stop]";
		std::cout << "\n";
	}
	else {
		std::cout << "Selected: nothing (camera only)\n";
	}
}

Selectable* ToyRoomApp::Selected()
{
	if (selectedId < 0 || selectedId >= static_cast<int>(selectables.size()))
		return nullptr;
	return &selectables[static_cast<size_t>(selectedId)];
}

Character* ToyRoomApp::DrivenCharacter()
{
	Selectable* s = Selected();
	if (!s || !s->character)
		return nullptr;
	if (s->character == jessie && jessieMounted)
		return bullseye; // the mounted unit is driven through the horse
	return s->character;
}

int ToyRoomApp::Pick(const glm::vec2& mouse) const
{
	int winW = 0, winH = 0;
	glfwGetWindowSize(window, &winW, &winH);
	if (winW <= 0 || winH <= 0)
		return -1;

	// Pixel -> normalised device coordinates -> world-space ray through the camera.
	const float x = 2.0f * mouse.x / static_cast<float>(winW) - 1.0f;
	const float y = 1.0f - 2.0f * mouse.y / static_cast<float>(winH);
	const float tanHalf = std::tan(glm::radians(camera.fov) * 0.5f);
	const float aspect = static_cast<float>(width) / static_cast<float>(height);
	Ray ray{ camera.position, glm::normalize(camera.Forward() + x * tanHalf * aspect * camera.Right() + y * tanHalf * camera.Up()) };

	float bestT = 1e30f;
	int bestOwner = -1;
	for (const DrawItem& item : renderer.Items()) {
		if (item.ownerId < 0)
			continue;
		const RayHit hit = RayIntersect::Object(item.mesh->Type(), glm::inverse(item.model), ray);
		if (hit.t > 0.0f && hit.t < bestT) {
			bestT = hit.t;
			bestOwner = item.ownerId;
		}
	}
	return bestOwner;
}

void ToyRoomApp::HandleCamera(float dt)
{
	if (input.Pressed(GLFW_KEY_C)) {
		camera.mode = static_cast<CameraMode>((static_cast<int>(camera.mode) + 1) % 3);
		if (camera.mode == CameraMode::Orbit) {
			// start orbiting from the current viewpoint
			const glm::vec3 offset = camera.position - camera.target;
			camera.orbitDistance = glm::length(offset);
			camera.orbitYaw = glm::degrees(std::atan2(offset.x, offset.z));
			camera.orbitPitch = glm::degrees(std::asin(std::clamp(offset.y / std::max(camera.orbitDistance, 1e-3f), -1.0f, 1.0f)));
		}
		std::cout << "Camera: " << CameraModeName(camera.mode) << "\n";
	}
	if (input.Pressed(GLFW_KEY_HOME)) {
		camera.Reset();
		std::cout << "Camera reset\n";
	}

	const Selectable* sel = Selected();
	const glm::vec3 focus = sel ? sel->node->WorldPosition() + glm::vec3(0.0f, sel->focusHeight, 0.0f) : glm::vec3(0.0f, 1.5f, 0.0f);

	if (input.Pressed(GLFW_KEY_F)) {
		camera.mode = CameraMode::Orbit;
		camera.target = focus;
		camera.orbitDistance = sel ? sel->focusDistance : 12.0f;
		std::cout << "Camera: Orbit around " << (sel ? sel->name : std::string("room")) << "\n";
	}

	// View presets (numpad, or Ctrl + 1/3/7)
	auto preset = [&](int numpadKey, int digitKey) {
		return input.Pressed(numpadKey) || (input.CtrlDown() && input.Pressed(digitKey));
	};
	if (preset(GLFW_KEY_KP_1, GLFW_KEY_1)) { camera.mode = CameraMode::Orbit; camera.orbitYaw = 0; camera.orbitPitch = 5; }
	if (preset(GLFW_KEY_KP_3, GLFW_KEY_3)) { camera.mode = CameraMode::Orbit; camera.orbitYaw = 90; camera.orbitPitch = 5; }
	if (preset(GLFW_KEY_KP_7, GLFW_KEY_7)) { camera.mode = CameraMode::Orbit; camera.orbitYaw = 0; camera.orbitPitch = 85; }

	const glm::vec2 md = input.MouseDelta();
	const bool rotating = input.MouseDown(GLFW_MOUSE_BUTTON_RIGHT);
	const bool panning = input.MouseDown(GLFW_MOUSE_BUTTON_MIDDLE);
	const float moveSpeed = (input.ShiftDown() ? 18.0f : 6.0f) * dt;

	switch (camera.mode) {
	case CameraMode::Free: {
		if (rotating) {
			camera.yaw += md.x * 0.15f;
			camera.pitch = std::clamp(camera.pitch - md.y * 0.15f, -89.0f, 89.0f);
		}
		glm::vec3 move(0.0f);
		if (input.Down(GLFW_KEY_UP)) move += camera.Forward();
		if (input.Down(GLFW_KEY_DOWN)) move -= camera.Forward();
		if (input.Down(GLFW_KEY_RIGHT)) move += camera.Right();
		if (input.Down(GLFW_KEY_LEFT)) move -= camera.Right();
		if (input.Down(GLFW_KEY_PAGE_UP)) move.y += 1.0f;
		if (input.Down(GLFW_KEY_PAGE_DOWN)) move.y -= 1.0f;
		if (glm::length(move) > 0.0f)
			camera.position += glm::normalize(move) * moveSpeed;
		if (panning)
			camera.position += (-camera.Right() * md.x + camera.Up() * md.y) * 0.01f;
		// Scroll = zoom by narrowing the field of view (optical zoom).
		camera.fov = std::clamp(camera.fov - input.Scroll() * 3.0f, 10.0f, 90.0f);
		camera.target = camera.position + camera.Forward() * 8.0f;
		break;
	}
	case CameraMode::Orbit: {
		if (!panning)
			camera.target = glm::mix(camera.target, focus, std::min(1.0f, dt * 6.0f));
		if (rotating) {
			camera.orbitYaw -= md.x * 0.3f;
			camera.orbitPitch += md.y * 0.3f;
		}
		if (input.Down(GLFW_KEY_LEFT)) camera.orbitYaw -= 90.0f * dt;
		if (input.Down(GLFW_KEY_RIGHT)) camera.orbitYaw += 90.0f * dt;
		if (input.Down(GLFW_KEY_UP)) camera.orbitPitch += 60.0f * dt;
		if (input.Down(GLFW_KEY_DOWN)) camera.orbitPitch -= 60.0f * dt;
		if (input.Down(GLFW_KEY_PAGE_UP)) camera.orbitDistance *= 1.0f - 1.5f * dt;
		if (input.Down(GLFW_KEY_PAGE_DOWN)) camera.orbitDistance *= 1.0f + 1.5f * dt;
		// Scroll = dolly (move closer / further) - lets you inspect any object up close.
		camera.orbitDistance *= std::pow(0.88f, input.Scroll());
		if (panning)
			camera.target += (-camera.Right() * md.x + camera.Up() * md.y) * 0.004f * camera.orbitDistance;
		camera.ApplyOrbit();
		break;
	}
	case CameraMode::Follow: {
		glm::vec3 back(0.0f, 0.0f, -1.0f);
		if (Character* c = DrivenCharacter())
			back = -c->Forward();
		else if (sel)
			back = AxisOf(sel->node, 2, -1.0f);
		camera.orbitDistance *= std::pow(0.88f, input.Scroll());
		camera.orbitDistance = std::clamp(camera.orbitDistance, 1.5f, 20.0f);
		const glm::vec3 desired = focus + back * camera.orbitDistance + glm::vec3(0.0f, camera.orbitDistance * 0.45f, 0.0f);
		camera.position = glm::mix(camera.position, desired, std::min(1.0f, dt * 4.0f));
		camera.target = focus;
		camera.LookAt(focus);
		break;
	}
	}
}

void ToyRoomApp::HandleObjectControl(float dt)
{
	Selectable* sel = Selected();
	if (!sel)
		return;

	// ---- Props --------------------------------------------------------------------------
	if (selectedId == lampId) {
		const float swivel = (input.Down(GLFW_KEY_A) ? 1.0f : 0.0f) - (input.Down(GLFW_KEY_D) ? 1.0f : 0.0f);
		const float tilt = (input.Down(GLFW_KEY_W) ? 1.0f : 0.0f) - (input.Down(GLFW_KEY_S) ? 1.0f : 0.0f);
		environment.DriveLamp(swivel, tilt, dt);
		if (input.Pressed(GLFW_KEY_R)) {
			environment.ToggleLamp();
			std::cout << "Lamp " << (environment.lampPower ? "ON" : "OFF") << "\n";
		}
		if (input.Down(GLFW_KEY_COMMA)) environment.lampBrightness = std::max(0.0f, environment.lampBrightness - dt);
		if (input.Down(GLFW_KEY_PERIOD)) environment.lampBrightness = std::min(5.0f, environment.lampBrightness + dt);
		return;
	}
	if (selectedId == ballId) {
		// Push relative to the camera so "W" always rolls the ball away from the viewer.
		glm::vec3 f = camera.Forward(); f.y = 0.0f;
		glm::vec3 r = camera.Right(); r.y = 0.0f;
		f = glm::normalize(f); r = glm::normalize(r);
		glm::vec3 push(0.0f);
		if (input.Down(GLFW_KEY_W)) push += f;
		if (input.Down(GLFW_KEY_S)) push -= f;
		if (input.Down(GLFW_KEY_D)) push += r;
		if (input.Down(GLFW_KEY_A)) push -= r;
		if (glm::length(push) > 0.0f)
			environment.PushBall(glm::normalize(push), dt);
		if (input.Pressed(GLFW_KEY_SPACE))
			environment.StopBall();
		return;
	}

	// ---- Characters ---------------------------------------------------------------------
	if (!sel->character)
		return;
	if ((sel->character == jessie || sel->character == bullseye) && input.Pressed(GLFW_KEY_R))
		ToggleMount();

	Character* driven = DrivenCharacter();
	if (!driven)
		return;

	ControlInput in;
	if (input.Down(GLFW_KEY_W)) in.forward += 1.0f;
	if (input.Down(GLFW_KEY_S)) in.forward -= 1.0f;
	if (input.Down(GLFW_KEY_A)) in.turn += 1.0f;
	if (input.Down(GLFW_KEY_D)) in.turn -= 1.0f;
	if (input.Down(GLFW_KEY_Q)) in.vertical += 1.0f;
	if (input.Down(GLFW_KEY_E)) in.vertical -= 1.0f;
	in.run = input.ShiftDown();

	if (input.Pressed(GLFW_KEY_SPACE)) {
		driven->Stop();
		std::cout << driven->Name() << " stopped\n";
	}
	if (input.Down(GLFW_KEY_SPACE))
		in = {}; // SPACE held: stay stopped
	if (input.Pressed(GLFW_KEY_L) && driven->SpecialName()) {
		driven->Special();
		std::cout << driven->Name() << ": " << driven->SpecialName() << " toggled\n";
	}
	driven->Drive(in, dt);
}

void ToyRoomApp::HandleEditMode(float dt)
{
	Selectable* sel = Selected();
	if (!sel)
		return;
	if (input.Pressed(GLFW_KEY_T)) {
		editOp = static_cast<EditOp>((static_cast<int>(editOp) + 1) % 4);
		std::cout << "Edit operation: " << EditOpName(editOp) << "\n";
	}

	Transform& t = sel->node->local;
	if (input.Pressed(GLFW_KEY_BACKSPACE)) {
		t = sel->initial;
		std::cout << sel->name << " transform reset\n";
		return;
	}
	if (input.Pressed(GLFW_KEY_M)) {
		// Reflection about the object's local YZ plane (mirror left <-> right).
		t.basis = t3d::reflect({ 1.0f, 0.0f, 0.0f }) * t.basis;
		std::cout << sel->name << " mirrored (reflection about local YZ plane)\n";
	}

	auto axisInput = [&](int neg, int pos) {
		return (input.Down(pos) ? 1.0f : 0.0f) - (input.Down(neg) ? 1.0f : 0.0f);
	};
	const glm::vec3 a(axisInput(GLFW_KEY_J, GLFW_KEY_L), axisInput(GLFW_KEY_O, GLFW_KEY_U), axisInput(GLFW_KEY_I, GLFW_KEY_K));
	if (a == glm::vec3(0.0f))
		return;
	const float boost = input.ShiftDown() ? 3.0f : 1.0f;

	switch (editOp) {
	case EditOp::Translate:
		t.position += a * 2.0f * boost * dt;
		break;
	case EditOp::Rotate:
		t.rotation += glm::vec3(a.z, a.x, a.y) * 90.0f * boost * dt; // I/K pitch, J/L yaw, U/O roll
		break;
	case EditOp::Scale: {
		if (input.CtrlDown()) {
			const float s = 1.0f + (a.x + a.y + a.z) * boost * dt;
			t.scale *= s;
		}
		else {
			t.scale *= glm::vec3(1.0f) + a * boost * dt;
		}
		t.scale = glm::clamp(t.scale, glm::vec3(0.05f), glm::vec3(10.0f));
		break;
	}
	case EditOp::Shear:
		// x += k*y (J/L), z += k*y (I/K), y += k*x (U/O)
		t.basis = t3d::shear(a.x * boost * dt, 0.0f, a.y * boost * dt, 0.0f, 0.0f, a.z * boost * dt) * t.basis;
		break;
	}
}

// =============================================================================================
// Mount / dismount (parent-child re-linking with world transform preservation)
// =============================================================================================
void ToyRoomApp::ToggleMount()
{
	if (jessie->InTransition())
		return;
	if (jessieMounted) {
		Dismount();
		return;
	}
	const glm::vec3 d = jessie->Root()->WorldPosition() - bullseye->Root()->WorldPosition();
	const float distance = glm::length(glm::vec2(d.x, d.z));
	constexpr float MountDistance = 2.2f;
	if (distance > MountDistance) {
		std::cout << "Jessie is too far from Bullseye (" << std::fixed << std::setprecision(1) << distance
		          << " > " << MountDistance << ").\n" << std::defaultfloat;
		return;
	}
	Mount();
}

void ToyRoomApp::Mount()
{
	SceneNode* j = jessie->Root();
	SceneNode* seat = bullseye->Seat();

	// 1. Remember Jessie's current WORLD matrix.
	const glm::mat4 world = j->World();
	// 2. Re-link: detach from the world root, attach under Bullseye's seat joint.
	seat->AttachChild(j->Parent()->DetachChild(j));
	// 3. Express the same world placement in the seat's coordinate system:
	//      world = seat.world * local   =>   local = inverse(seat.world) * world
	const glm::mat4 local = glm::inverse(seat->World()) * world;
	j->local.position = glm::vec3(local[3]);
	j->local.rotation.y = glm::degrees(std::atan2(local[2].x, local[2].z));
	// 4. Glide from there onto the saddle (hips on the seat, facing forward).
	jessie->clampToRoom = false;
	jessie->Stop();
	jessie->SetSeated(true);
	jessie->StartTransition(glm::vec3(0.0f, -Humanoid::HipHeight, 0.0f), 0.0f, 0.7f);
	jessieMounted = true;
	std::cout << "Jessie mounted Bullseye.\n";
}

void ToyRoomApp::Dismount()
{
	SceneNode* j = jessie->Root();
	SceneNode* seat = bullseye->Seat();

	// World placement while seated (computed through the whole hierarchy).
	const glm::mat4 world = j->World();
	scene->AttachChild(seat->DetachChild(j));
	// The root's parent is the identity, so local = world.
	j->local.position = glm::vec3(world[3]);
	j->local.rotation.y = glm::degrees(std::atan2(world[2].x, world[2].z));

	// Step down on Bullseye's left side, facing the same way he does.
	const float h = glm::radians(bullseye->Heading());
	const glm::vec3 left(std::cos(h), 0.0f, -std::sin(h));
	glm::vec3 target = bullseye->Root()->WorldPosition() + left * 1.1f;
	target.y = 0.0f;
	jessie->SetSeated(false);
	jessie->clampToRoom = true;
	jessie->StartTransition(target, bullseye->Heading(), 0.6f);
	jessieMounted = false;
	std::cout << "Jessie dismounted.\n";
}

// =============================================================================================
// Lights follow their scene nodes
// =============================================================================================
void ToyRoomApp::UpdateLights()
{
	const RoomRig& rig = environment.Rig();

	Light& sky = lights[SkyLight];
	sky.direction = environment.skyLightDirection;
	sky.color = environment.skyLightColor;
	sky.intensity = environment.skyLightIntensity;

	const glm::vec3 lampPos = rig.lampLightAnchor->WorldPosition();
	const glm::vec3 lampDir = AxisOf(rig.lampHead, 1, -1.0f); // shade opens along the head's -Y
	lights[LampBulb].position = lampPos;
	lights[LampBulb].intensity = environment.lampIntensity * 0.5f;
	lights[LampSpot].position = lampPos;
	lights[LampSpot].direction = lampDir;
	lights[LampSpot].intensity = environment.lampIntensity * 1.6f;

	for (int i = 0; i < 2; ++i) {
		Light& l = lights[static_cast<size_t>(HeadlightL + i)];
		const SceneNode* anchor = car->HeadlightAnchor(i);
		l.position = anchor->WorldPosition();
		glm::vec3 dir = AxisOf(anchor, 2, 1.0f);
		dir.y -= 0.15f;
		l.direction = glm::normalize(dir);
		l.enabled = car->HeadlightsOn();
		l.intensity = 1.5f;
	}

	lights[LaserGlow].enabled = buzz->LaserOn();
	lights[LaserGlow].position = buzz->LaserTip();
	lights[LaserGlow].intensity = 1.5f;

	lights[GhostGlow].position = rig.ghost->WorldPosition();
	lights[GhostGlow].intensity = 0.8f * environment.ghostVisibility;
	lights[GhostGlow].enabled = environment.ghostVisibility > 0.02f;
}

// =============================================================================================
// Render
// =============================================================================================
void ToyRoomApp::OnRender()
{
	renderer.Collect(*scene, camera.position);

	FrameInfo frame;
	frame.camera = &camera;
	frame.aspect = static_cast<float>(width) / static_cast<float>(height);
	frame.lights = &lights;
	frame.ambientLight = environment.AmbientLight();
	frame.clearColor = environment.ClearColor();
	frame.selectedOwner = selectedId;
	frame.time = static_cast<float>(time);

	if (settings.rayTracing) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		rayTracer.Render(renderer.Items(), frame, settings, renderer, width, height);
	}
	else {
		renderer.Render(frame, settings);

		// Debug overlay: local axes of the selected object (X red, Y green, Z blue) - shows its
		// model coordinate frame, and what rotation/scale/shear does to it.
		if (settings.showGizmo) {
			if (const Selectable* sel = Selected()) {
				const glm::mat4& m = sel->node->World();
				const glm::vec3 o(m[3]);
				const glm::vec3 lift(0.0f, 0.02f, 0.0f);
				renderer.Lines().Add(o + lift, o + lift + glm::vec3(m[0]) * 1.2f, { 1.0f, 0.2f, 0.2f });
				renderer.Lines().Add(o + lift, o + lift + glm::vec3(m[1]) * 1.2f, { 0.2f, 1.0f, 0.2f });
				renderer.Lines().Add(o + lift, o + lift + glm::vec3(m[2]) * 1.2f, { 0.3f, 0.5f, 1.0f });
			}
		}
		renderer.RenderDebug(frame, settings);
	}

	// Scripted capture (--capture): read the finished back buffer before it is swapped.
	if (!launch.capture.empty() && ++frameCounter == launch.frames) {
		SaveScreenshot(launch.capture);
		Close();
	}
}

// =============================================================================================
// Command line
// =============================================================================================
LaunchOptions LaunchOptions::Parse(int argc, char* argv[])
{
	LaunchOptions o;
	auto floats = [](const std::string& s) {
		std::vector<float> v;
		std::stringstream ss(s);
		std::string item;
		while (std::getline(ss, item, ','))
			v.push_back(std::stof(item));
		return v;
	};
	for (int i = 1; i < argc; ++i) {
		const std::string a = argv[i];
		const bool hasValue = i + 1 < argc;
		if (a == "--hour" && hasValue) o.hour = std::stof(argv[++i]);
		else if (a == "--select" && hasValue) o.select = std::stoi(argv[++i]);
		else if (a == "--focus") o.focus = true;
		else if (a == "--mount") o.mount = true;
		else if (a == "--raytrace") o.rayTrace = true;
		else if (a == "--shading" && hasValue) o.shading = std::stoi(argv[++i]);
		else if (a == "--wireframe") o.wireframe = true;
		else if (a == "--normals") o.normals = true;
		else if (a == "--pause") o.pauseClock = true;
		else if (a == "--story") o.story = true;
		else if (a == "--capture" && hasValue) o.capture = argv[++i];
		else if (a == "--frames" && hasValue) o.frames = std::stoi(argv[++i]);
		else if (a == "--cam" && hasValue) {
			const auto v = floats(argv[++i]);
			if (v.size() == 6) {
				o.hasCamera = true;
				o.cameraPos = { v[0], v[1], v[2] };
				o.cameraTarget = { v[3], v[4], v[5] };
			}
		}
		else if (a == "--orbit" && hasValue) {
			const auto v = floats(argv[++i]);
			if (v.size() == 3) { o.orbitYaw = v[0]; o.orbitPitch = v[1]; o.orbitDistance = v[2]; }
		}
	}
	return o;
}

void ToyRoomApp::ApplyLaunchOptions()
{
	if (launch.hour >= 0.0f) environment.hour = launch.hour;
	if (launch.pauseClock || !launch.capture.empty()) environment.paused = true;
	if (!launch.capture.empty() && !launch.story) environment.hauntingEnabled = false; // deterministic poses
	if (launch.rayTrace) settings.rayTracing = true;
	if (launch.shading >= 0 && launch.shading <= 3) settings.shading = static_cast<ShadingMode>(launch.shading);
	settings.wireframe = launch.wireframe;
	settings.showNormals = launch.normals;
	if (launch.mount) {
		// place Jessie beside Bullseye, then mount through the normal code path
		const float h = glm::radians(bullseye->Heading());
		jessie->Root()->local.position = bullseye->Root()->local.position + glm::vec3(std::cos(h), 0.0f, -std::sin(h)) * 1.2f;
		scene->UpdateWorld(glm::mat4(1.0f));
		Mount();
	}
	if (launch.select >= 0 && launch.select < static_cast<int>(selectables.size()))
		Select(launch.select);
	if (launch.hasCamera) {
		camera.position = launch.cameraPos;
		camera.LookAt(launch.cameraTarget);
	}
	if (launch.focus || launch.orbitDistance > 0.0f) {
		const Selectable* sel = Selected();
		camera.mode = CameraMode::Orbit;
		camera.target = sel ? sel->node->WorldPosition() + glm::vec3(0.0f, sel->focusHeight, 0.0f) : glm::vec3(0.0f, 1.5f, 0.0f);
		camera.orbitDistance = launch.orbitDistance > 0.0f ? launch.orbitDistance : (sel ? sel->focusDistance : 12.0f);
		if (launch.orbitYaw < 1e8f) camera.orbitYaw = launch.orbitYaw;
		if (launch.orbitPitch < 1e8f) camera.orbitPitch = launch.orbitPitch;
		camera.ApplyOrbit();
	}
}

// =============================================================================================
// Text output
// =============================================================================================
void ToyRoomApp::UpdateTitle()
{
	std::ostringstream t;
	t << "Haunted Toy Room | " << environment.ClockText() << (environment.paused ? " (paused)" : "")
	  << " | Selected: ";
	if (const Selectable* s = Selected()) {
		t << s->name;
		if (s->character == jessie && jessieMounted) t << " + Bullseye";
	}
	else {
		t << "none";
	}
	if (editMode) t << " [EDIT: " << EditOpName(editOp) << "]";
	t << " | Camera: " << CameraModeName(camera.mode)
	  << " | " << (settings.rayTracing ? "Ray traced" : ToString(settings.shading))
	  << (settings.wireframe ? " wireframe" : "")
	  << " | " << static_cast<int>(fps + 0.5f) << " FPS";
	if (settings.rayTracing)
		t << " (" << rayTracer.InstanceCount() << " primitives, x" << settings.rayScale << ")";
	SetTitle(t.str());
}

void ToyRoomApp::PrintHelp() const
{
	std::cout <<
		"\n==================== HAUNTED TOY ROOM ====================\n"
		"SELECT      1 Woody  2 Jessie  3 Bullseye  4 Buzz  5 RC Car  6 Ball  7 Lamp  8 Ghost\n"
		"            0 nothing   |  left-click any object to select it\n"
		"CHARACTER   W/S move  A/D turn  Shift run  SPACE stop\n"
		"            R  Jessie mount / dismount Bullseye (must be close)\n"
		"            Q/E Buzz fly up/down   L Buzz laser / Car headlights\n"
		"LAMP        A/D swivel  W/S tilt  R power  ,/. brightness\n"
		"BALL        W/S/A/D push (camera relative)  SPACE stop\n"
		"CAMERA      C cycle Free/Orbit/Follow   F focus+orbit selected   Home reset\n"
		"            Free:  arrows move, PgUp/PgDn up/down, right-drag look, scroll = zoom (FOV)\n"
		"            Orbit: right-drag or arrows rotate, scroll/PgUp/PgDn distance, middle-drag pan\n"
		"            Numpad 1/3/7 (or Ctrl+1/3/7) front/side/top view\n"
		"EDIT MODE   Tab toggle   T op (Translate/Rotate/Scale/Shear)   J/L X  U/O Y  I/K Z\n"
		"            Ctrl = uniform scale   M mirror (reflect)   Backspace reset   Shift faster\n"
		"RENDER      F1 wireframe  F2 shading (Flat/Gouraud/Phong/Blinn)  F3 textures\n"
		"            F4 RAY TRACING  (-/= resolution, 9 bounces)\n"
		"            F5 ambient  F6 diffuse  F7 specular  F8 sun/moon light\n"
		"            F9 normals  F10 vertices  F11 axes gizmo  F12 screenshot (BMP)\n"
		"            V list parts of selected object, Shift+V dump vertex/index tables\n"
		"WORLD       P pause clock  [ ] time speed  , . scrub time  N story/haunting on/off\n"
		"            H help   Esc quit\n"
		"==========================================================\n\n";
}

void ToyRoomApp::DumpSelectedGeometry(bool full) const
{
	if (selectedId < 0) {
		std::cout << "Select an object first.\n";
		return;
	}
	const Selectable& sel = selectables[static_cast<size_t>(selectedId)];
	size_t parts = 0, vertices = 0, triangles = 0;
	std::set<const Mesh*> used;
	std::cout << "\n--- " << sel.name << " : parts ---\n";
	sel.node->ForEach([&](SceneNode& n) {
		if (!n.mesh || n.ownerId != selectedId)
			return;
		++parts;
		vertices += n.mesh->Data().vertices.size();
		triangles += n.mesh->TriangleCount();
		used.insert(n.mesh);
		const glm::vec3 wp = n.WorldPosition();
		std::cout << "  " << std::left << std::setw(16) << n.name << std::setw(9) << n.mesh->Name()
		          << " local pos(" << std::fixed << std::setprecision(2) << n.local.position.x << ", " << n.local.position.y << ", " << n.local.position.z
		          << ") size(" << n.local.scale.x << ", " << n.local.scale.y << ", " << n.local.scale.z
		          << ") world(" << wp.x << ", " << wp.y << ", " << wp.z << ")\n" << std::defaultfloat;
	});
	std::cout << "  total: " << parts << " parts, " << vertices << " vertices, " << triangles << " triangles\n";

	if (!full)
		return;
	for (const Mesh* mesh : used) {
		const MeshData& d = mesh->Data();
		std::cout << "\n--- " << mesh->Name() << " : " << d.vertices.size() << " vertices ---\n"
		          << "  idx     position (x, y, z)          normal (x, y, z)        uv\n";
		for (size_t i = 0; i < d.vertices.size(); ++i) {
			const Vertex& v = d.vertices[i];
			std::printf("  %4zu  (%6.3f %6.3f %6.3f)  (%6.3f %6.3f %6.3f)  (%5.3f %5.3f)\n", i,
				v.position.x, v.position.y, v.position.z, v.normal.x, v.normal.y, v.normal.z, v.uv.x, v.uv.y);
		}
		std::cout << "--- " << mesh->Name() << " : " << mesh->TriangleCount() << " triangles (indices) ---\n";
		for (size_t i = 0; i + 2 < d.indices.size(); i += 3)
			std::printf("  T%-4zu %4u %4u %4u\n", i / 3, d.indices[i], d.indices[i + 1], d.indices[i + 2]);
	}
	std::cout << std::flush;
}

void ToyRoomApp::SaveScreenshot(const std::string& path)
{
	Image img(width, height);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, img.pixels.data());
	std::string name = path;
	if (name.empty()) {
		std::filesystem::create_directories("screenshots");
		char buf[64];
		std::snprintf(buf, sizeof(buf), "screenshots/shot_%03d.bmp", screenshotCounter++);
		name = buf;
	}
	if (Bmp::Save(name, img))
		std::cout << "Saved " << std::filesystem::absolute(name).string() << "\n";
	else
		std::cout << "Could not save screenshot\n";
}
