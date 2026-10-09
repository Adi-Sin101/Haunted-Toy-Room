#include "ToyRoomApp.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "characters/Bullseye.h"
#include "characters/Humanoid.h"
#include "characters/RCCar.h"
#include "characters/Cat.h"
#include "math/Ray.h"
#include "render/BmpLoader.h"
#include "world/Room.h"

namespace {

// Fixed slots in the light list.
enum LightSlot : int { SkyLight = 0, LampBulb, LampSpot, HallGlow, HeadlightL, HeadlightR, LaserGlow, GhostGlow, LightSlotCount };

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

ToyRoomApp::ToyRoomApp(const LaunchOptions& options) : Application(options.windowWidth, options.windowHeight, "Haunted Toy Room"), launch(options)
{
	assets.Load();
	renderer.Init(assets);
	rayTracer.Init(assets);
	hud.Init();
	BuildScene();
	camera.Reset();
	PrintHelp();
	ApplyLaunchOptions();
}

ToyRoomApp::~ToyRoomApp() { if (recording) std::fclose(recording); }

// =============================================================================================
// Scene construction
// =============================================================================================
void ToyRoomApp::BuildScene()
{
	scene = std::make_unique<SceneNode>("World");
	const RoomRig rig = BuildRoom(*scene, assets);
	// The big wooden crate stands tidily against the left wall, out of every walking route.
	rig.missionObstacle->local.position = {-9.0f, 1.0f, 6.9f};
	environment.Init(rig);

	// Home positions = where the toys stand once they are out: a tidy line in front of the toy chest.
	auto woodyPtr = std::make_unique<Humanoid>(*scene, assets, WoodyStyle(), glm::vec3(-1.6f, 0.0f, 3.4f), 10.0f);
	auto jessiePtr = std::make_unique<Humanoid>(*scene, assets, JessieStyle(), glm::vec3(0.7f, 0.0f, 3.7f), -5.0f);
	auto bullseyePtr = std::make_unique<Bullseye>(*scene, assets, glm::vec3(3.4f, 0.0f, 3.1f), 60.0f);
	auto buzzPtr = std::make_unique<Buzz>(*scene, assets, glm::vec3(-3.4f, 0.0f, 0.4f), 35.0f);
	auto carPtr = std::make_unique<RCCar>(*scene, assets, glm::vec3(5.5f, 0.0f, -1.8f), -90.0f);
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
	for (size_t i = 0; i < rig.blocks.size(); ++i)
		AddSelectable(rig.blocks[i]==rig.missionObstacle ? "Doorway crate" : "Wooden block " + std::to_string(i), rig.blocks[i], nullptr, 0, 0.0f, 2.4f);

	// The house around the toy room, the story props, and Penny the cat.
	house = BuildHouse(*scene, assets);
	storyProps = BuildStoryProps(*scene, assets);
	penny = std::make_unique<Cat>(*scene, assets, glm::vec3(2.0f, RoomSize::Ground, 23.4f), 90.0f);
	pennyId = static_cast<int>(selectables.size());
	AddSelectable("Penny", penny->Root(), penny.get(), 0, 0.5f, 3.0f);
	for (const char* name : {"Desk", "DeskChair", "Bed", "Bookcase", "CeilingFan", "WallClock", "Window", "Poster", "Rug", "Moon", "ToyChest", "Wardrobe", "BuzzRoomBed"}) {
		if (SceneNode* node=scene->Find(name)) AddSelectable(name,node,nullptr,0,0.5f,4.0f);
	}
	arrival.Init(penny.get(), house);
	hallwayPuzzle.Build(*scene, assets, house);
	StoryDirector::Hooks hooks;
	hooks.mount = [this]() { scene->UpdateWorld(glm::mat4(1)); if (!jessieMounted) Mount(); };
	hooks.dismount = [this]() { scene->UpdateWorld(glm::mat4(1)); if (jessieMounted) Dismount(); };
	hooks.mounted = [this]() { return jessieMounted; };
	story.Init(penny.get(), woody, jessie, bullseye, buzz, car, &physics, hooks);
	story.Build(*scene, assets, house, storyProps);
	// Everyone uses the house-aware PhysicsWorld bounds rather than the room-only clamp in Character.
	penny->clampToRoom = false;
	for (Character* c : std::vector<Character*>{woody, jessie, bullseye, buzz, car}) c->clampToRoom = false;
	if (SceneNode* leaf = house.frontDoor ? house.frontDoor->Find("FrontDoorLeaf") : nullptr)
		leaf->solid = true;

	characters.push_back(std::move(woodyPtr));
	characters.push_back(std::move(jessiePtr));
	characters.push_back(std::move(bullseyePtr));
	characters.push_back(std::move(buzzPtr));
	characters.push_back(std::move(carPtr));

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
	lights[HallGlow] = { "Hall night light", LightType::Point, true, false };
 lights[HallGlow].position={13.5f,3.7f,4}; lights[HallGlow].color={0.75f,0.80f,1.0f};
 lights[HallGlow].intensity=0.85f; lights[HallGlow].linear=0.10f; lights[HallGlow].quadratic=0.03f;
 lights[GhostGlow] = { "Ghost glow", LightType::Point, true, false };
	lights[GhostGlow].color = { 0.5f, 0.7f, 1.0f };
	lights[GhostGlow].linear = 0.3f;
	lights[GhostGlow].quadratic = 0.15f;

	scene->UpdateWorld(glm::mat4(1.0f));
	auto bodies=rig.blocks; bodies.insert(bodies.end(),story.Debris().begin(),story.Debris().end());
    physics.Init(*scene,bodies);
    physics.EnableHouse(true);
	physics.EnableHallway(true);
	for (const auto& c : characters) {
		glm::vec3 half(0.40f, 1.08f, 0.60f), offset(0, 1.08f, 0);
		if (c.get() == bullseye) { half = {0.55f, 1.30f, 0.55f}; offset = {0, 1.30f, 0.0f}; }
		if (c.get() == car) { half = {0.64f, 0.65f, 0.85f}; offset = {0, 0.65f, 0}; }
		physics.AddActor(c->Root(), half, offset);
		Material& shadow = assets.Mat("contact-shadow", {0.012f, 0.018f, 0.03f}, 0.0f);
		shadow.unlit = true; shadow.opacity = 0.20f;
		contactShadows.push_back(scene->AddShape("ContactShadow", &assets.Sphere(), &shadow,
			c->Root()->local.position + glm::vec3(0, 0.012f, 0), {half.x * 2.6f, 0.012f, half.z * 2.6f}));
	}
	physics.AddActor(rig.ball, glm::vec3(rig.ballRadius), glm::vec3(0));
	physics.AddActor(rig.ghost, {0.60f, 0.85f, 0.60f}, glm::vec3(0));
	physics.AddActor(rig.lamp, {0.55f, 0.90f, 0.75f}, {0, 0.90f, 0});
	physics.AddActor(penny->Root(), {0.34f, 0.48f, 0.34f}, {0, 0.48f, 0});
	// Spines (see PhysicsWorld::Actor): Penny reaches from her tail at -0.8 to her nose at +1.0, Bullseye from
	// his rump at -0.9 to his muzzle at +1.7. Square segments cover that length at any heading.
	physics.SetActorSpine(penny->Root(), {-0.46f, 0.10f, 0.64f});
	physics.SetActorSpine(bullseye->Root(), {-0.35f, 0.45f, 1.15f});
	physics.EnableActor(penny->Root(), false); // Arrival controls her route until Stage 1 begins.
 story.Restart(true);
 environment.hauntingEnabled=false;
 scene->UpdateWorld(glm::mat4(1));
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
 const int steps=launch.story && !launch.capture.empty() ? launch.storySteps : 1;
 if (!launch.record.empty()) dt = 1.0f / launch.recordFps;
 else if (!launch.capture.empty()) dt = 1.0f / 60.0f;
 for (int i=0;i<steps;++i) { StepScene(dt); if (i+1<steps) input.EndFrame(); }
}
void ToyRoomApp::StepScene(float dt)
{
	if (launch.storyStep > 0 && !launch.capture.empty()) dt=launch.storyStep;
 if (!launch.capture.empty() || !launch.record.empty()) { simulationTime += dt; time = simulationTime; }
	glm::vec3 previousCamera = camera.position;
	glm::vec3 previousPenny=penny->Root()->WorldPosition();
	previousPositions.resize(characters.size());
	for (size_t i=0;i<characters.size();++i) previousPositions[i]=characters[i]->Root()->WorldPosition();
	glm::vec3 previousBall = environment.Rig().ball->local.position;
	glm::vec3 previousGhost = environment.Rig().ghost->local.position;
	glm::vec3 previousLamp = environment.Rig().lamp->local.position;
	statusTimer = std::max(0.0f, statusTimer - dt);
	HandleGlobalKeys();
 if (missionRestarted) {
  previousCamera=camera.position;
  previousPenny=penny->Root()->WorldPosition();
  previousBall=environment.Rig().ball->local.position;
  previousGhost=environment.Rig().ghost->local.position;
  previousLamp=environment.Rig().lamp->local.position;
  for (size_t i=0;i<characters.size();++i) previousPositions[i]=characters[i]->Root()->WorldPosition();
  missionRestarted=false;
 }
	// Penny's arrival owns the camera and the clock; the toys stay frozen until it ends (Y skips it).
	if (arrival.Active() && input.Pressed(GLFW_KEY_Y)) arrival.Skip();
	const bool arriving = arrival.Active();
	if (!arriving) {
		if (story.enabled && !story.SelectionOpen())
			selectedId = pennyId; // only Penny exists until the toys come alive
		else
			HandleSelection();
		HandleCamera(dt);
		Character* driver=story.enabled ? DrivenCharacter() : nullptr;
		story.SetControlled(driver,driver==bullseye && jessieMounted ? jessie : nullptr);
		if ((!story.enabled || (Selected() && !cinematic)) && !hallwayPuzzle.KeypadActive()) {
			if (editMode) HandleEditMode(dt); else HandleObjectControl(dt);
		}
	}

 if (!arriving) {
  Character* driver=story.enabled ? DrivenCharacter() : nullptr;
  story.SetControlled(driver,driver==bullseye && jessieMounted ? jessie : nullptr);
  physics.SetIndependentActor(driver ? driver->Root() : nullptr);
 }
 if (arriving) { arrival.Update(dt); environment.hour = arrival.Hour(); }
 else {
  if (!gameplayStarted) {
   // The establishing shot is over: hand Penny to the player in the garden (story) or the hallway (sandbox).
   gameplayStarted = true;
   penny->EnableArrivalMotion(false);
   hallwayPuzzle.Begin();
   if (story.Sandbox()) hallwayPuzzle.Open();
   penny->StopLooking();
   penny->SetPose(Cat::Pose::Walk);
   previousPenny = penny->Root()->local.position;
   if (house.stairDoor) house.stairDoor->local.rotation.y = 90.0f; // Keep the stair landing visible and connected.
   selectedId = pennyId;
   editMode = false;
   story.enabled = !launch.manual; // Input owns Penny; the director continues the other actors.
   if (launch.manual && launch.select>=0) selectedId=launch.select;
   storyCamera = false;
   camera.mode = CameraMode::Follow;
		// Begin with a centered third-person view down the real corridor, leaving the
		// stair landing and clue route readable around Penny.
		camera.orbitYaw = 0.0f;
		camera.orbitPitch = 18.0f;
		camera.orbitDistance = story.Sandbox() ? 4.0f : 4.6f;
   camera.target = penny->Root()->local.position + glm::vec3(0.0f, 0.65f, 0.0f);
   camera.position = story.Sandbox() ? glm::vec3(16.3f, 2.0f, 2.4f) : camera.target - penny->Forward() * 4.4f + glm::vec3(0.0f, 1.5f, 0.0f);
   if (const Selectable* chosen = Selected(); chosen && selectedId != pennyId) {
    // A manual launch can select a toy in the bedroom; start behind it instead of in the hallway.
    // Use the follow offset directly so the camera never has to pass through the actor's own body.
    const Character* driven = DrivenCharacter();
    const glm::vec3 back = driven ? -driven->Forward() : glm::vec3(0.0f, 0.0f, 1.0f);
    camera.target = chosen->node->WorldPosition() + glm::vec3(0.0f, chosen->focusHeight, 0.0f);
    camera.position = camera.target + back * 3.8f + glm::vec3(0.0f, 1.24f, 0.0f);
   }
   camera.LookAt(camera.target);
   previousCamera = camera.position;
   scene->UpdateWorld(glm::mat4(1.0f));
  }
 }
 HandlePuzzleInteraction();
 hallwayPuzzle.Update(dt);
 // Followers ignore each other's bodies (they keep formation slots) but still collide with the house.
 physics.SetScriptedActors(story.FollowersActive());
 const bool userPaused=environment.paused;
 const bool outside=CameraOutdoors();
 environment.outdoors=outside;
 // Walking through the front door eases the light over ~0.3 s; a camera jump (launch, restart, a new
 // selection) snaps it, or the house would briefly throw the window light's long shadow over the garden.
 if (glm::distance(camera.position,outdoorEye)>2.0f) outdoorBlend=outside ? 1.0f : 0.0f;
 else outdoorBlend+=((outside ? 1.0f : 0.0f)-outdoorBlend)*(1.0f-std::exp(-dt*8.0f));
 outdoorEye=camera.position; if (story.enabled && !story.Sandbox()) { environment.paused=true; if (!arriving) environment.hour=story.Hour(); }
 environment.Update(dt,static_cast<float>(time),selectedId==lampId,selectedId==ballId,selectedId==ghostId);
 environment.paused=userPaused;
 if (!userPaused && environment.Rig().fanRotor) {
  auto& angle=environment.Rig().fanRotor->local.rotation.y;
  angle=std::fmod(angle+dt*110.0f,360.0f);
 }

    if (!arriving && !userPaused && launch.demo) DemoMission(dt);
    if (!arriving && !userPaused) story.Update(dt);
    scene->UpdateWorld(glm::mat4(1));
	if (!arriving) // during the arrival the toys keep the rest pose the story's restart gave them
		for (auto& c : characters)
			c->Animate(dt, static_cast<float>(time));
	// Penny uses the existing character controller once selected; otherwise she watches the active toy from her current position.
	if (!arriving) {
		if (DrivenCharacter() == penny.get() || penny->IsMoving()) {
			penny->SetPose(Cat::Pose::Walk); // driven by the player, or following the toys
			penny->StopLooking();
		} else {
			penny->SetPose(Cat::Pose::Sit);
			const Character* focus = buzz->LaserOn() ? static_cast<const Character*>(buzz) : jessieMounted ? static_cast<const Character*>(bullseye) : woody;
			if (focus->Root()->visible) penny->LookAt(focus->Root()->WorldPosition() + glm::vec3(0.0f, 1.0f, 0.0f));
		}
	}
	penny->Animate(dt, static_cast<float>(time));
	house.exterior->visible = arrival.ExteriorVisible() || (story.ExteriorVisible() && !story.Sandbox());
	house.interior->visible = true;
	// The sky dome (sky.glsl) now draws the sun, infinitely far along the light's own direction; the old
	// garden sun sphere sat at a fixed point, so its direction drifted from the light as the camera moved.
	house.outdoorSun->visible = false;
	for (const auto& c : characters) // toys still hidden in the chest or wardrobe are not bodies yet
		physics.EnableActor(c->Root(), !story.Hidden(c.get()) && !(c.get() == jessie && jessieMounted));
	physics.EnableActor(penny->Root(), !arriving);
	physics.EnableActor(environment.Rig().ghost, environment.Rig().ghost->visible);
	const bool buzzAirborne = buzz->Root()->local.position.y > PhysicsWorld::FloorHeight(buzz->Root()->local.position)+0.06f;
    physics.SetActorGrounded(buzz->Root(),false);
	physics.SetActorShape(buzz->Root(), {buzzAirborne ? 0.92f : 0.42f, buzzAirborne ? 0.60f : 1.08f, buzzAirborne ? 1.0f : 0.60f}, {0, buzzAirborne ? 0.60f : 1.08f, 0});
	// Square spine segments (set up in the constructor): their rotated bounds stay narrow enough for
	// Bullseye to turn in the 3-unit corridor and pass the bedroom doorway, while the chain still reaches
	// from rump to muzzle (a single 2.6-long box could not turn there at all).
	physics.SetActorShape(bullseye->Root(), {0.55f, jessieMounted ? 1.75f : 1.30f, 0.55f}, {0, jessieMounted ? 1.75f : 1.30f, 0.0f});
	for (size_t i = 0; i < characters.size(); ++i) {
		Character* c = characters[i].get();
		if (c == jessie && jessieMounted) continue;
		// Mounting hops, climbing out of the chest and Buzz's first flight are scripted transitions.
		if (c->InTransition() || story.Hidden(c) || story.Scripted(c)) continue;
		const glm::vec3 wanted = c->Root()->local.position;
		physics.ConstrainActor(c->Root(), previousPositions[i]);
		if (c == DrivenCharacter() && glm::distance(wanted, c->Root()->local.position) > 0.04f) {
			statusText = "Solid contact / turn or choose another path"; statusTimer = 1.0f;
		}
	}
	if (!arriving) {
		const glm::vec3 wanted = penny->Root()->local.position;
		physics.ConstrainActor(penny->Root(), previousPenny);
		if (DrivenCharacter() == penny.get() && glm::distance(wanted, penny->Root()->local.position) > 0.04f) {
			statusText = "Solid contact / turn or choose another path"; statusTimer = 1.0f;
		}
	}
	// Characters never stand inside each other. Scripted moments (the chest climb, mounting hops, Buzz's
	// first flight) are left alone; a body flying above another is not in contact (PhysicsWorld checks height).
	{
		// Right of way, highest first: the character the player drives, characters on a story task,
		// Penny (the group's leader), then the followers in a fixed order.
		Character* anchor = DrivenCharacter();
		auto eligible = [&](Character* c) {
			if (c == penny.get()) return !arriving && !penny->InTransition();
			return !(c->InTransition() || story.Hidden(c) || story.Scripted(c) || (c == jessie && jessieMounted));
		};
		const std::vector<Character*> order = {bullseye, buzz, jessie, woody, car};
		std::vector<SceneNode*> bodies;
		auto add = [&](Character* c) {
			if (c && eligible(c) && std::find(bodies.begin(), bodies.end(), c->Root()) == bodies.end()) bodies.push_back(c->Root());
		};
		add(anchor);
		for (Character* c : order) if (story.Busy(c)) add(c);
		add(penny.get());
		for (Character* c : order) add(c);
		physics.SeparateActors(bodies, anchor ? anchor->Root() : nullptr);
	}
	physics.ConstrainActor(environment.Rig().ball, previousBall);
	physics.ConstrainActor(environment.Rig().ghost, previousGhost);
	physics.ConstrainActor(environment.Rig().lamp, previousLamp);
	physics.Update(dt);
	for (size_t i = 0; i < contactShadows.size(); ++i) {
		const glm::vec3 p = characters[i]->Root()->local.position;
		const float support = physics.FloorHeight(p);
		// Contact cues belong on the current tread or floor, not the upstairs y=0 plane.
		// An airborne or elevated actor has no contact with this support surface.
		contactShadows[i]->local.position = {p.x, support + 0.012f, p.z};
		contactShadows[i]->visible = characters[i]->Root()->visible && settings.lighting && settings.shadingEnabled && !settings.rayTracing &&
			!(characters[i].get() == jessie && jessieMounted) && std::abs(p.y - support) < 0.06f;
	}

	// One depth-first pass computes every world matrix: world = parent.world * local.
	scene->UpdateWorld(glm::mat4(1.0f));
	if (buzz->LaserReady()) {
		buzz->SetLaserLength(physics.FireLaser(buzz->LaserTip(), buzz->LaserDirection(), dt, buzz->Root()));
		scene->UpdateWorld(glm::mat4(1.0f));
	}
	if (arriving) {
		// The arrival camera moves freely through the garden and the house; the room's camera
		// constraints apply again at the upper hallway.
		camera.mode = CameraMode::Free;
		camera.position = arrival.CameraPosition();
		camera.LookAt(arrival.CameraTarget());
	}
	else {
		if (camera.mode != CameraMode::Free) {
			const Selectable* selected = Selected();
			SceneNode* ignored = selected ? selected->node : nullptr;
			glm::vec3 sight = physics.CameraSightline(camera.target, camera.position, ignored);
			// Pull in at once when a wall cuts the view, but move back out gradually: in the stairwell
			// the railings and the landing wall block and clear the view on alternate frames, and
			// taking each answer as-is made the camera jump in and out.
			const glm::vec3 offset = camera.position - camera.target;
			const float full = glm::length(offset), allowed = glm::distance(sight, camera.target);
			if (full > 1e-3f) {
				if (sightReach < 0.0f || allowed < sightReach || camera.mode != CameraMode::Follow) sightReach = allowed;
				else sightReach += (allowed - sightReach) * (1.0f - std::exp(-dt * 2.5f));
				sight = camera.target + offset / full * std::min(sightReach, full);
			}
			glm::vec3 moved = physics.MoveCamera(previousCamera, sight);
			// After a selection change the camera can sit on the far side of a wall from its new
			// target, and sliding can never carry it through. Once it has no clear view of the
			// target, jump to the unobstructed sightline position on the target's side.
			if (glm::distance(physics.CameraSightline(camera.target, moved, ignored), moved) > 0.3f) moved = sight;
			camera.position = moved;
			camera.LookAt(camera.target);
		}
		else { camera.position = physics.MoveCamera(previousCamera, camera.position); sightReach = -1.0f; }
	}
	// Story shots (the morning pull-back over the house) take over the camera, then hand it back.
	glm::vec3 shotPosition, shotTarget;
	if (!arriving && story.enabled && story.Cinematic(shotPosition, shotTarget)) {
		if (!cinematic) { cinematic = true; cinematicTarget = camera.target; }
		camera.mode = CameraMode::Free;
		const float k = 1.0f - std::exp(-dt * 0.9f);
		camera.position = glm::mix(camera.position, shotPosition, k);
		cinematicTarget = glm::mix(cinematicTarget, shotTarget, k);
		camera.target = cinematicTarget;
		camera.LookAt(cinematicTarget);
	} else if (cinematic) {
		cinematic = false;
		camera.mode = CameraMode::Follow;
	}
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

	const bool cancelKeypad = hallwayPuzzle.KeypadActive() && input.Pressed(GLFW_KEY_ESCAPE);
	if (cancelKeypad) {
		hallwayPuzzle.CancelKeypad();
		statusText = hallwayPuzzle.Status(); statusTimer = 3.0f;
	} else if (hallwayPuzzle.KeypadActive()) {
		for (int digit = 0; digit <= 9; ++digit) {
			if (input.Pressed(GLFW_KEY_0 + digit) && hallwayPuzzle.AddDigit(digit)) {
				statusText = hallwayPuzzle.CodeDisplay(); statusTimer = 3.0f;
			}
		}
		if (input.Pressed(GLFW_KEY_BACKSPACE) && hallwayPuzzle.Backspace()) {
			statusText = hallwayPuzzle.CodeDisplay(); statusTimer = 3.0f;
		}
		if (input.Pressed(GLFW_KEY_ENTER)) {
			const PuzzleCode::Result result = hallwayPuzzle.Submit();
			if (result == PuzzleCode::Result::Correct)
				story.Advance(StoryDirector::Transition::PUZZLE_SOLVED);
			statusText = hallwayPuzzle.Status(); statusTimer = 4.0f;
			std::cout << "PUZZLE " << statusText << "\n";
		}
	}

	if (input.Pressed(GLFW_KEY_ESCAPE) && !cancelKeypad) {
		if (cursorCaptured || mouseLook) {
			mouseLook=false; releaseMouseUntilButtonUp=true; SetCursorCaptured(false);
		} else Close();
	}
	if (!editMode && input.Pressed(GLFW_KEY_M)) {
		mouseLook=!mouseLook;
		if (mouseLook) { storyCamera=false; renderSettingsOpen=false; }
	}
	if (input.Pressed(GLFW_KEY_H)) { helpVisible = !helpVisible; PrintHelp(); }
	if (input.Pressed(GLFW_KEY_G)) hudVisible = !hudVisible;
	if (input.Pressed(GLFW_KEY_GRAVE_ACCENT)) {
		renderSettingsOpen=!renderSettingsOpen;
		if (renderSettingsOpen) hudVisible=true;
		if (renderSettingsOpen) { mouseLook=false; releaseMouseUntilButtonUp=true; SetCursorCaptured(false); }
	}
	if (input.Pressed(GLFW_KEY_B)) {
		if (input.CtrlDown()) {
			const int count = static_cast<int>(physics.Bodies().size());
			if (count > 0) Select(8 + (selectedId >= 8 ? (selectedId - 8 + 1) % count : 0));
		} else { physics.ResetBlocks(); statusText = "The block tower has been rebuilt"; statusTimer = 3.0f; }
	}

	if (input.Pressed(GLFW_KEY_F1)) { settings.rayTracing=false; flip(settings.wireframe, "Wireframe"); }
 if (!editMode && input.Pressed(GLFW_KEY_O)) flip(environment.hauntingEnabled, "Haunted ambience");
	if (input.Pressed(GLFW_KEY_F2)) {
  settings.rayTracing=false; // Gouraud is a raster interpolation technique.
		if (input.ShiftDown()) settings.shadingEnabled=!settings.shadingEnabled;
		else if (!settings.shadingEnabled) settings.shadingEnabled=true;
		else settings.shading = static_cast<ShadingMode>((static_cast<int>(settings.shading) + 1) % 4);
		std::cout << "Shading: " << ToString(settings.shading) << "\n";
	}
	if (input.Pressed(GLFW_KEY_F3)) flip(settings.textures, "Textures");
	if (input.Pressed(GLFW_KEY_F4)) {
		flip(settings.rayTracing, "Ray tracing");
		if (settings.rayTracing)
			std::cout << "  (resolution x" << settings.rayScale << ", bounces " << settings.rayBounces << "; -/= resolution, Ctrl+9 bounces)\n";
	}
	if (input.Pressed(GLFW_KEY_F5)) flip(settings.ambient, "Ambient");
	if (input.Pressed(GLFW_KEY_F6)) flip(settings.diffuse, "Diffuse");
	if (input.Pressed(GLFW_KEY_F7)) flip(settings.specular, "Specular");
	if (input.Pressed(GLFW_KEY_F8)) {
		lights[SkyLight].enabled = !lights[SkyLight].enabled;
		std::cout << "Sun/Moon light: " << (lights[SkyLight].enabled ? "ON" : "OFF") << "\n";
	}
	if (input.Pressed(GLFW_KEY_F9)) { settings.rayTracing=false; flip(settings.showNormals, "Normals of selected object"); }
	if (input.Pressed(GLFW_KEY_F10)) { settings.rayTracing=false; flip(settings.showVertices, "Vertices of selected object"); }
	if (input.Pressed(GLFW_KEY_F11)) { settings.rayTracing=false; flip(settings.showGizmo, "Local axes gizmo"); }
	if (input.Pressed(GLFW_KEY_F12)) SaveScreenshot();
	if (input.Pressed(GLFW_KEY_V)) DumpSelectedGeometry(input.ShiftDown());

	if (input.Pressed(GLFW_KEY_MINUS)) settings.rayScale = std::max(0.2f, settings.rayScale - 0.1f);
	if (input.Pressed(GLFW_KEY_EQUAL)) settings.rayScale = std::min(1.0f, settings.rayScale + 0.1f);
	if (!hallwayPuzzle.KeypadActive() && input.Pressed(GLFW_KEY_9) && input.CtrlDown()) { // plain 9 takes / releases Penny
		settings.rayBounces = (settings.rayBounces + 1) % 5;
		std::cout << "Ray bounces: " << settings.rayBounces << "\n";
	}

	// World clock and story
	if (input.Pressed(GLFW_KEY_P)) {
  flip(environment.paused,"Story / clock paused"); if (environment.paused && story.enabled) story.Pause();
 }
	if (input.Pressed(GLFW_KEY_N)) {
  if (input.ShiftDown()) {
   // Replay the whole story from the street, whatever mode the session started in.
   story.Restart(true); arrival.Restart(); hallwayPuzzle.Reset();
   for (int id : {ballId,lampId,ghostId}) selectables[static_cast<size_t>(id)].node->local=selectables[static_cast<size_t>(id)].initial;
   scene->UpdateWorld(glm::mat4(1)); camera.Reset(); missionRestarted=true;
   demoPhase=demoLeg=0;demoWait=0; gameplayStarted=false; launch.manual=false; cinematic=false;
   environment.lampPower=true;environment.lampManual=false;environment.hauntingEnabled=false;environment.paused=false;
  }
  else if (story.CurrentState() == StoryDirector::GameplayState::PUZZLE && story.enabled) {
   story.enabled = false;
   selectedId = pennyId;
  }
  else { story.enabled=!story.enabled; if (!story.enabled) story.Pause(); }
  if (story.enabled) {
   editMode=false;
   selectedId=pennyId;
  }
  statusText=story.enabled ? "Gameplay progression active" : "Manual control / N resumes gameplay"; statusTimer=3;
  std::cout<<statusText<<"\n";
 }
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
		// Editing a selected object also preserves independent story playback.
		std::cout << "Edit mode " << (editMode ? "ON" : "OFF");
		if (editMode) std::cout << " (" << EditOpName(editOp) << ") - T operation, J/L X, U/O Y, I/K Z, M mirror, Backspace reset";
		std::cout << "\n";
	}
}

void ToyRoomApp::HandlePuzzleInteraction()
{
    if (arrival.Active() || hallwayPuzzle.KeypadActive() || !story.enabled) return;
    // L fires Buzz's laser from the story layer while he waits in position and the player drives
    // someone without an L action of their own (Penny, Woody, Jessie).
    Character* driven = DrivenCharacter();
    if (input.Pressed(GLFW_KEY_L) && (!driven || !driven->SpecialName()) && story.RequestLaser()) {
        statusText = "Buzz fires his laser at the door!"; statusTimer = 4;
        return;
    }
    if (!input.Pressed(GLFW_KEY_ENTER)) return;
    if (story.PuzzleAvailable() && driven==penny.get()) {
        statusText=hallwayPuzzle.Interact(penny->Root()->WorldPosition());
        if (hallwayPuzzle.KeypadActive()) penny->Stop();
    } else statusText=story.Interact(driven);
    statusTimer=7;
    if (!statusText.empty()) std::cout<<"INTERACT "<<statusText<<"\n";
}

void ToyRoomApp::HandleSelection()
{
	if (story.enabled && !story.SelectionOpen()) {
		selectedId = pennyId;
		return; // the toys become selectable once they are alive
	}
	if (!hallwayPuzzle.KeypadActive()) {
        if (input.Pressed(GLFW_KEY_0) && input.CtrlDown()) Select(pennyId);
        if (input.Pressed(GLFW_KEY_9) && !input.CtrlDown()) TogglePennyControl();
		if (input.Pressed(GLFW_KEY_0) && !input.CtrlDown())
			Select(-1);
		for (int i = 0; i < static_cast<int>(selectables.size()); ++i) {
			const int key = selectables[static_cast<size_t>(i)].key;
			if (key > 0 && !input.CtrlDown() && input.Pressed(GLFW_KEY_0 + key))
				Select(i);
		}
	}
	if (hallwayPuzzle.KeypadActive()) return;

	// Mouse picking: a left click (not a drag) casts a ray into the scene.
	if (input.MousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
		mouseDownPos = input.MousePosition();
		mouseDragDistance = 0.0f;
	}
	if (input.MouseDown(GLFW_MOUSE_BUTTON_LEFT))
		mouseDragDistance += glm::length(input.MouseDelta());
	if (input.MouseReleased(GLFW_MOUSE_BUTTON_LEFT) && mouseDragDistance < 4.0f) {
		if (Selected() && Selected()->character == buzz && (input.Down(GLFW_KEY_LEFT_ALT) || input.Down(GLFW_KEY_RIGHT_ALT))) return;
		int winW, winH; glfwGetWindowSize(window, &winW, &winH);
		if (hudVisible && !cursorCaptured) {
			const int button = hud.HitTest(input.MousePosition(), winW, winH, renderSettingsOpen);
			if (button>=100) {
				if (button==Hud::PennyButton) TogglePennyControl();
				if (button==100) renderSettingsOpen=!renderSettingsOpen;
				if (button==101) settings.lighting=!settings.lighting;
				if (button==102) settings.shadingEnabled=!settings.shadingEnabled;
				if (button==103) settings.rayTracing=!settings.rayTracing;
				if (button==104) settings.textures=!settings.textures;
				return;
			}
			if (button >= 0) {
				const int count = static_cast<int>(physics.Bodies().size());
				Select(button == 8 && count > 0 ? 8 + (selectedId >= 8 ? (selectedId - 8 + 1) % count : 0) : button);
				return;
			}
			if (hud.Covers(input.MousePosition(), winW, winH, helpVisible, renderSettingsOpen)) return;
		}
		const int id = Pick(input.MousePosition());
		if (id >= 0)
			Select(id);
	}
}

void ToyRoomApp::Select(int id)
{
	if (id == selectedId)
		return;
	if (id >= 0 && story.enabled && !story.Available(selectables[static_cast<size_t>(id)].character)) {
		statusText = selectables[static_cast<size_t>(id)].name + " is still trapped"; statusTimer = 3;
		std::cout << statusText << "\n";
		return;
	}
	if (Character* c = DrivenCharacter())
		c->Stop();
	selectedId = id;
	if (id>=0 && story.enabled) {
		// A rider and horse must detach before either receives independent ownership.
		const Character* actor=selectables[static_cast<size_t>(id)].character;
		if (jessieMounted && (actor==jessie || actor==bullseye)) Dismount();
		storyCamera=false;
		statusText="Live control / other toys continue / 0 releases / N full manual"; statusTimer=4;
	}
	cameraPan = glm::vec3(0);
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
        if (story.enabled && s->character) std::cout<<"Live control: "<<s->name<<"\n";
	}
	else {
        if (story.enabled) std::cout<<"Live control: released to simulation\n";
		std::cout << "Selected: nothing (camera only)\n";
	}
}

void ToyRoomApp::TogglePennyControl()
{
	if (selectedId == pennyId) {
		Select(-1);
		statusText = "Penny is back in the simulation / 9 or the PENNY button takes her again"; statusTimer = 4;
		return;
	}
	editMode = false;
	Select(pennyId);
	if (selectedId != pennyId) return;
	// She may be anywhere (another floor, outside the view): put the follow camera behind her at once.
	camera.mode = CameraMode::Follow;
	camera.orbitYaw = 0.0f; camera.orbitPitch = 18.0f; camera.orbitDistance = 4.6f;
	cameraPan = glm::vec3(0.0f);
	storyCamera = false;
	statusText = "You control Penny / W/S move / A/D turn / 9 releases her"; statusTimer = 4;
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

Ray ToyRoomApp::ViewRay(const glm::vec2& mouse) const
{
	if (cursorCaptured) return {camera.position,camera.Forward()};
	int winW = 0, winH = 0;
	glfwGetWindowSize(window, &winW, &winH);
	if (winW <= 0 || winH <= 0)
		return {camera.position, camera.Forward()};

	// Pixel -> normalised device coordinates -> world-space ray through the camera.
	const float x = 2.0f * mouse.x / static_cast<float>(winW) - 1.0f;
	const float y = 1.0f - 2.0f * mouse.y / static_cast<float>(winH);
	const float tanHalf = std::tan(glm::radians(camera.fov) * 0.5f);
	const float aspect = static_cast<float>(width) / static_cast<float>(height);
	return { camera.position, glm::normalize(camera.Forward() + x * tanHalf * aspect * camera.Right() + y * tanHalf * camera.Up()) };
}

int ToyRoomApp::Pick(const glm::vec2& mouse) const
{
	const Ray ray = ViewRay(mouse);

	float bestT = 1e30f;
	int bestOwner = -1;
	for (const DrawItem& item : renderer.Items()) {
		if (item.material->opacity < 0.5f)
			continue;
		const RayHit hit = RayIntersect::Object(item.mesh->Type(), glm::inverse(item.model), ray);
		if (hit.t > 0.0f && hit.t < bestT) {
			bestT = hit.t;
			bestOwner = item.ownerId;
		}
	}
	return bestOwner;
}

void ToyRoomApp::SetCursorCaptured(bool capture)
{
	if (cursorCaptured==capture) return;
	cursorCaptured=capture;
	if (glfwRawMouseMotionSupported()) glfwSetInputMode(window,GLFW_RAW_MOUSE_MOTION,capture ? GLFW_TRUE : GLFW_FALSE);
	glfwSetInputMode(window,GLFW_CURSOR,capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	input.ResetMouseMotion(); // ignore cursor warps on capture/release
}

void ToyRoomApp::HandleCamera(float dt)
{
 if (!glfwGetWindowAttrib(window,GLFW_FOCUSED)) mouseLook=false;
 if (!input.MouseDown(GLFW_MOUSE_BUTTON_RIGHT)) releaseMouseUntilButtonUp=false;
 if (editMode) mouseLook=false;
 const bool capture=glfwGetWindowAttrib(window,GLFW_FOCUSED)
  && (mouseLook || (input.MouseDown(GLFW_MOUSE_BUTTON_RIGHT) && !releaseMouseUntilButtonUp));
 SetCursorCaptured(capture);
 const bool freeWasd=camera.mode==CameraMode::Free && selectedId<0 && !editMode;
 if (cursorCaptured || (freeWasd && (input.Down(GLFW_KEY_W) || input.Down(GLFW_KEY_A) || input.Down(GLFW_KEY_S) || input.Down(GLFW_KEY_D)))) storyCamera=false;
 if (input.MouseDown(GLFW_MOUSE_BUTTON_RIGHT) || input.MouseDown(GLFW_MOUSE_BUTTON_MIDDLE) || input.Scroll()!=0
  || input.Down(GLFW_KEY_LEFT) || input.Down(GLFW_KEY_RIGHT) || input.Down(GLFW_KEY_UP) || input.Down(GLFW_KEY_DOWN)
  || input.Down(GLFW_KEY_PAGE_UP) || input.Down(GLFW_KEY_PAGE_DOWN) || input.Pressed(GLFW_KEY_C)
  || input.Pressed(GLFW_KEY_F) || input.Pressed(GLFW_KEY_HOME)) storyCamera=false;
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
		cameraPan = glm::vec3(0);
		std::cout << "Camera reset\n";
	}

	const Selectable* sel = Selected();
	const glm::vec3 focus = sel ? sel->node->WorldPosition() + glm::vec3(0.0f, sel->focusHeight, 0.0f) : glm::vec3(0.0f, 1.5f, 0.0f);
	if (story.CurrentState() == StoryDirector::GameplayState::PUZZLE && camera.mode == CameraMode::Follow
		&& DrivenCharacter() == penny.get()) {
		const glm::vec3 p = penny->Root()->WorldPosition();
		const bool userOrbiting = cursorCaptured || input.Down(GLFW_KEY_LEFT) || input.Down(GLFW_KEY_RIGHT)
			|| input.Down(GLFW_KEY_UP) || input.Down(GLFW_KEY_DOWN);
		if (!userOrbiting) {
			// Keep the follow camera centered behind Penny. Diagonal offsets caused the narrow
			// hallway walls and clock to dominate the view while she approached clues.
			const glm::vec3 back = -penny->Forward();
			auto clearance = [&](float yaw) {
				const float angle = glm::radians(yaw);
				const glm::vec3 direction(back.x * std::cos(angle) + back.z * std::sin(angle), 0.0f,
					back.z * std::cos(angle) - back.x * std::sin(angle));
				float distance = 4.8f;
				if (direction.x > 0.001f) distance = std::min(distance, (RoomSize::HallEnd - 0.55f - p.x) / direction.x);
				else if (direction.x < -0.001f) distance = std::min(distance, (p.x - RoomSize::HalfWidth - 0.55f) / -direction.x);
				if (direction.z > 0.001f) distance = std::min(distance, (RoomSize::DoorHigh - 0.55f - p.z) / direction.z);
				else if (direction.z < -0.001f) distance = std::min(distance, (p.z - RoomSize::DoorLow - 0.55f) / -direction.z);
				return std::max(0.0f, distance);
			};
			const float northClearance = clearance(-35.0f);
			const float southClearance = clearance(35.0f);
			// Change sides only when the other side is clearly roomier: with equal clearances the
			// camera used to flip between -35 and +35 degrees every few frames and shook.
			const bool north = camera.orbitYaw < 0.0f;
			if (std::abs(camera.orbitYaw) != 35.0f) camera.orbitYaw = northClearance >= southClearance ? -35.0f : 35.0f;
			else if (north && southClearance > northClearance + 0.6f) camera.orbitYaw = 35.0f;
			else if (!north && northClearance > southClearance + 0.6f) camera.orbitYaw = -35.0f;
			const float roomy = camera.orbitYaw < 0.0f ? northClearance : southClearance;
			const float wanted = std::clamp(roomy - 0.25f, 1.8f, 4.8f);
			camera.orbitDistance += (wanted - camera.orbitDistance) * (1.0f - std::exp(-dt * 3.0f));
		}
	}

	if (input.Pressed(GLFW_KEY_F)) {
		cameraPan = glm::vec3(0);
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
	const bool rotating = cursorCaptured;
	const bool panning = input.MouseDown(GLFW_MOUSE_BUTTON_MIDDLE);
	const float zoomSpeed = std::clamp(camera.fov / 55.0f, 0.25f, 1.5f);
	const float moveSpeed = (input.ShiftDown() ? 10.0f : 4.0f) * zoomSpeed * dt;
	const float scroll = input.CtrlDown() ? 0.0f : input.Scroll();
	if (input.CtrlDown()) camera.fov = std::clamp(camera.fov - input.Scroll() * 2.0f, 12.0f, 85.0f);

	switch (camera.mode) {
	case CameraMode::Free: {
		if (rotating) {
			camera.yaw += md.x * 0.15f * zoomSpeed;
			camera.yaw=WrapDegrees(camera.yaw);
			camera.pitch = std::clamp(camera.pitch - md.y * 0.15f * zoomSpeed, -88.0f, 88.0f);
		}
		glm::vec3 move(0.0f);
		if (input.Down(GLFW_KEY_UP)) move += camera.Forward();
		if (input.Down(GLFW_KEY_DOWN)) move -= camera.Forward();
		if (input.Down(GLFW_KEY_RIGHT)) move += camera.Right();
		if (input.Down(GLFW_KEY_LEFT)) move -= camera.Right();
		if (input.Down(GLFW_KEY_PAGE_UP)) move.y += 1.0f;
		if (input.Down(GLFW_KEY_PAGE_DOWN)) move.y -= 1.0f;
		if (freeWasd) {
			glm::vec3 forward=camera.Forward(); forward.y=0;
			forward=glm::normalize(forward);
			if (input.Down(GLFW_KEY_W)) move+=forward;
			if (input.Down(GLFW_KEY_S)) move-=forward;
			if (input.Down(GLFW_KEY_D)) move+=camera.Right();
			if (input.Down(GLFW_KEY_A)) move-=camera.Right();
		}
		if (glm::length(move) > 0.0f)
			camera.position += glm::normalize(move) * moveSpeed;
		if (panning)
			camera.position += (-camera.Right() * md.x + camera.Up() * md.y) * 0.01f;
		// Scroll = zoom by narrowing the field of view (optical zoom).
		camera.fov = std::clamp(camera.fov - scroll * 3.0f, 12.0f, 85.0f);
		camera.target = camera.position + camera.Forward() * 8.0f;
		break;
	}
	case CameraMode::Orbit: {
		if (panning) cameraPan += (-camera.Right() * md.x + camera.Up() * md.y) * 0.004f * camera.orbitDistance * zoomSpeed;
		camera.target = glm::mix(camera.target, focus + cameraPan, std::min(1.0f, dt * 6.0f));
		camera.target = glm::clamp(camera.target, glm::vec3(-22.0f, RoomSize::Ground + 0.3f, -RoomSize::HalfDepth + 0.3f),
			glm::vec3(30.0f, RoomSize::Height - 0.3f, 34.0f)); // the whole house and its garden
		if (rotating) {
			camera.orbitYaw -= md.x * 0.3f * zoomSpeed;
			camera.orbitPitch += md.y * 0.3f * zoomSpeed;
		}
		if (input.Down(GLFW_KEY_LEFT)) camera.orbitYaw -= 90.0f * dt;
		if (input.Down(GLFW_KEY_RIGHT)) camera.orbitYaw += 90.0f * dt;
		if (input.Down(GLFW_KEY_UP)) camera.orbitPitch += 60.0f * dt;
		if (input.Down(GLFW_KEY_DOWN)) camera.orbitPitch -= 60.0f * dt;
		if (input.Down(GLFW_KEY_PAGE_UP)) camera.orbitDistance *= 1.0f - 1.5f * dt;
		if (input.Down(GLFW_KEY_PAGE_DOWN)) camera.orbitDistance *= 1.0f + 1.5f * dt;
		// Scroll = dolly (move closer / further) - lets you inspect any object up close.
		camera.orbitDistance *= std::pow(0.88f, scroll);
		camera.orbitYaw=WrapDegrees(camera.orbitYaw);
		camera.ApplyOrbit();
		break;
	}
	case CameraMode::Follow: {
		glm::vec3 back(0.0f, 0.0f, -1.0f);
		if (Character* c = DrivenCharacter())
			back = -c->Forward();
		else if (sel)
			back = AxisOf(sel->node, 2, -1.0f);
		if (rotating) { camera.orbitYaw -= md.x * 0.3f * zoomSpeed; camera.orbitPitch += md.y * 0.3f * zoomSpeed; }
		if (input.Down(GLFW_KEY_LEFT)) camera.orbitYaw -= 70.0f * dt;
		if (input.Down(GLFW_KEY_RIGHT)) camera.orbitYaw += 70.0f * dt;
		if (input.Down(GLFW_KEY_UP)) camera.orbitPitch += 45.0f * dt;
		if (input.Down(GLFW_KEY_DOWN)) camera.orbitPitch -= 45.0f * dt;
		if (input.Down(GLFW_KEY_PAGE_UP)) camera.orbitDistance *= 1.0f - dt;
		if (input.Down(GLFW_KEY_PAGE_DOWN)) camera.orbitDistance *= 1.0f + dt;
		camera.orbitPitch = std::clamp(camera.orbitPitch, -70.0f, 80.0f);
		camera.orbitDistance *= std::pow(0.88f, scroll);
		camera.orbitDistance = std::clamp(camera.orbitDistance, 1.5f, 20.0f);
		camera.orbitYaw=WrapDegrees(camera.orbitYaw);
		const float angle = glm::radians(camera.orbitYaw), pitch = glm::radians(camera.orbitPitch);
		if (panning) cameraPan += (-camera.Right() * md.x + camera.Up() * md.y) * 0.004f * camera.orbitDistance * zoomSpeed;
		back = glm::vec3(back.x * std::cos(angle) + back.z * std::sin(angle), 0, back.z * std::cos(angle) - back.x * std::sin(angle));
		// Ease the focus after the character. Height eases slowest: stair and porch treads lift the
		// body 0.17 units at a time, and aiming at the raw position jerked the view on every step.
		// A large jump (new selection, restart, teleport) snaps instead of sweeping across the house.
		if (!followFocusValid || glm::distance(followFocus, focus) > 2.5f) followFocus = focus;
		else {
			const float across = 1.0f - std::exp(-dt * 14.0f), up = 1.0f - std::exp(-dt * 5.0f);
			followFocus.x += (focus.x - followFocus.x) * across;
			followFocus.z += (focus.z - followFocus.z) * across;
			followFocus.y += (focus.y - followFocus.y) * up;
		}
		followFocusValid = true;
		const glm::vec3 desired = followFocus + cameraPan + back * std::cos(pitch) * camera.orbitDistance + glm::vec3(0, std::sin(pitch) * camera.orbitDistance, 0);
		camera.position = glm::mix(camera.position, desired, 1.0f - std::exp(-dt * 4.0f));
		camera.target = followFocus + cameraPan;
		camera.LookAt(camera.target);
		break;
	}
	}
	if (camera.mode != CameraMode::Follow) followFocusValid = false;
}

void ToyRoomApp::HandleObjectControl(float dt)
{
	if (hallwayPuzzle.KeypadActive()) return;
	Selectable* sel = Selected();
	if (!sel)
		return;
	if (physics.IsBlock(sel->node)) {
		glm::vec3 forward = camera.Forward(); forward.y = 0;
		if (glm::length(forward) < 0.01f) forward = {0, 0, -1}; else forward = glm::normalize(forward);
		const glm::vec3 right = camera.Right();
		glm::vec3 direction = forward * ((input.Down(GLFW_KEY_W) ? 1.0f : 0.0f) - (input.Down(GLFW_KEY_S) ? 1.0f : 0.0f))
			+ right * ((input.Down(GLFW_KEY_D) ? 1.0f : 0.0f) - (input.Down(GLFW_KEY_A) ? 1.0f : 0.0f));
		physics.PushBlock(sel->node, direction * dt * 9.0f);
		if (input.Down(GLFW_KEY_SPACE)) physics.StopBlock(sel->node);
		return;
	}

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
	in.forward=launch.scriptedDrive;
	in.turn=launch.scriptedTurn;
	in.vertical=launch.scriptedFly;
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
	if (driven == buzz) {
		buzz->TiltLaser(((input.Down(GLFW_KEY_Z) ? 1.0f : 0.0f) - (input.Down(GLFW_KEY_X) ? 1.0f : 0.0f)) * 45.0f * dt);
		if ((input.Down(GLFW_KEY_LEFT_ALT) || input.Down(GLFW_KEY_RIGHT_ALT)) && input.MouseDown(GLFW_MOUSE_BUTTON_LEFT)) {
			const Ray ray = ViewRay(input.MousePosition()); float distance = 40.0f;
			for (const DrawItem& item : renderer.Items()) {
				if (item.ownerId == 3 || item.material->opacity < 0.9f) continue;
				const float hit = RayIntersect::Object(item.mesh->Type(), glm::inverse(item.model), ray).t;
				if (hit > 0 && hit < distance) distance = hit;
			}
			buzz->AimAt(ray.origin + ray.direction * distance);
		}
	}
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
	physics.EnableActor(jessie->Root(), false);
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
	target.y = PhysicsWorld::FloorHeight({target.x, bullseye->Root()->WorldPosition().y, target.z});
	jessie->SetSeated(false);
	jessie->clampToRoom = false;
	jessie->StartTransition(target, bullseye->Heading(), 0.6f);
	jessieMounted = false;
	physics.EnableActor(jessie->Root(), true);
	std::cout << "Jessie dismounted.\n";
}

// =============================================================================================
// Lights follow their scene nodes
// =============================================================================================
// Outside = the exterior exists this frame (not the sandbox) and the eye is beyond the house's outer
// walls or above its roof. The garage and the porch count as outside.
bool ToyRoomApp::CameraOutdoors() const
{
	if (!house.exterior || !house.exterior->visible) return false;
	using namespace RoomSize;
	const glm::vec3 eye = camera.position;
	return eye.x < HouseLeft || eye.x > HouseRight || eye.z < HouseBack || eye.z > HouseFront || eye.y > Height + 5.5f;
}

void ToyRoomApp::UpdateLights()
{
	const RoomRig& rig = environment.Rig();

	Light& sky = lights[SkyLight];
	// Indoors the light keeps the window's direction; outdoors it comes from the visible sun or moon.
	sky.direction = glm::normalize(glm::mix(environment.skyLightDirection, environment.outdoorLightDirection, outdoorBlend));
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

	// The house light belongs to the space the camera is in: the upstairs hall's night light, the
	// ground-floor corridor lamp, the nightstand lamp in Buzz's room, or the porch lantern outside.
	Light& area = lights[HallGlow];
	const glm::vec3 eye = camera.position;
	area.enabled = true;
	if (eye.z > RoomSize::HouseFront) {
		area.name = "Porch lantern"; area.position = {10.5f, RoomSize::Ground + 2.6f, RoomSize::HouseFront + 0.5f};
		area.color = {1.0f, 0.85f, 0.6f}; area.intensity = 0.9f * environment.Night(); area.linear = 0.08f; area.quadratic = 0.02f;
	} else if (eye.y < RoomSize::Slab && eye.x < RoomSize::CorridorLeft) {
		area.name = "Bedroom lamp"; area.position = storyProps.nightLamp;
		area.color = {1.0f, 0.78f, 0.50f}; area.intensity = 1.35f; area.linear = 0.10f; area.quadratic = 0.04f;
	} else if (eye.y < RoomSize::Slab) {
		area.name = "Corridor lamp"; area.position = {12.0f, RoomSize::Slab - 0.5f, std::clamp(eye.z, -7.0f, 7.0f)};
		area.color = {1.0f, 0.90f, 0.75f}; area.intensity = 0.9f; area.linear = 0.10f; area.quadratic = 0.03f;
	} else {
		area.name = "Hall night light"; area.position = {13.5f, 3.7f, 4.0f};
		area.color = {0.75f, 0.80f, 1.0f}; area.intensity = 0.85f; area.linear = 0.10f; area.quadratic = 0.03f;
	}

	// The ghost's glow when it haunts; otherwise the story's pulsing light (chest button, wardrobe).
	glm::vec3 glowPosition, glowColor; float glowIntensity = 0.0f;
	if (environment.ghostVisibility > 0.02f) {
		lights[GhostGlow].position = rig.ghost->WorldPosition();
		lights[GhostGlow].color = {0.5f, 0.7f, 1.0f};
		lights[GhostGlow].intensity = 0.8f * environment.ghostVisibility;
		lights[GhostGlow].enabled = true;
	} else if (story.enabled && story.Glow(glowPosition, glowColor, glowIntensity)) {
		lights[GhostGlow].position = glowPosition;
		lights[GhostGlow].color = glowColor;
		lights[GhostGlow].intensity = glowIntensity;
		lights[GhostGlow].enabled = true;
	} else lights[GhostGlow].enabled = false;
	if (launch.lightOnly >= 0)
		for (size_t i=0;i<lights.size();++i) lights[i].enabled = static_cast<int>(i)==launch.lightOnly;
}

// =============================================================================================
// Render
// =============================================================================================
void ToyRoomApp::DrawHud()
{
	HudInfo info;
	info.settingsOpen=renderSettingsOpen;
	info.mouseLook=cursorCaptured;
	info.lighting=settings.lighting; info.shading=settings.shadingEnabled;
	info.rayTracing=settings.rayTracing; info.textures=settings.textures;
	info.clock = environment.ClockText(); info.paused = environment.paused;
	info.help = helpVisible; info.edit = editMode;
	// Buttons 1-8 are selectables 0-7; "B" stands for the blocks only (Penny and the furniture have higher ids too).
	info.selected = selectedId < 8 ? selectedId : (Selected() && physics.IsBlock(Selected()->node) ? 8 : -1);
	info.camera = CameraModeName(camera.mode); info.fps = fps;
	info.rendering = settings.rayTracing ? "RAY TRACING / SHADOWS + REFLECTIONS" : std::string("RASTER + LAMP SHADOWS / ") + ToString(settings.shading);
	info.status = statusTimer > 0 ? statusText : (buzz->LaserOn() ? "Buzz's laser is active / B rebuilds the block tower" : "");
	info.selection = "Explore the room";
	info.description = "Gameplay is paused. Select a toy to move; N resumes progression.";
	info.controls = "Click Penny / 1-8 select / Enter interact / H guide / G interface";
	if (const Selectable* selected = Selected()) {
		info.selection = selected->name;
		if (selected->character) info.controls = selected->character->ControlsHelp();
		if (selected->character == woody) info.description = "The sheriff: plaid cotton, stitched denim, a leather holster and a gold badge.";
		else if (selected->character == jessie) {
			info.description = jessieMounted ? "Jessie rides with Bullseye. Their transforms and movement stay connected." : "The cowgirl: a braided ponytail and embroidered clothes. Walk near Bullseye to ride.";
			info.controls = "W/S move / A/D turn / Shift run / Space stop / R mount or dismount";
		} else if (selected->character == bullseye) {
			info.description = "A jointed toy horse with leather tack. Jessie can mount the saddle.";
			info.controls += " / R mount or dismount";
		} else if (selected->character == buzz) {
			info.description = "The space ranger: fly, aim at the block tower and watch each impact topple it.";
			info.controls = "W/S move / A/D turn / Q/E fly / L laser / Z/X aim / Alt+click target";
		} else if (selected->character == car) {
   info.description="A driveable RC car: axle-driven wheels and moving spotlight headlights.";
   info.controls="W/S drive / A/D steer / L headlights";
  }
		else if (selectedId == ballId) {
			info.description = "The beach ball rolls across the floor; furniture and the room walls stop it.";
			info.controls = "W/A/S/D roll relative to your view / Space stop / F inspect";
		} else if (selectedId == lampId) {
			info.description = "The articulated desk lamp follows nightfall, flickers and scans the room.";
			info.controls = "A/D swivel / W/S tilt / R power / , and . brightness";
		} else if (selectedId == ghostId) {
			info.description = "A translucent visitor appears at night. Inspect or reposition it in edit mode.";
			info.controls = "Tab edit / F inspect / O haunting on or off";
		} else if (physics.IsBlock(selected->node)) {
			info.description = "A solid wooden block. Gravity, other blocks and Buzz's laser affect its motion.";
			info.controls = "W/A/S/D push / Space brake / B rebuild / Ctrl+B next block";
		}
		if (editMode) info.controls = std::string("T ") + EditOpName(editOp) + " / J-L X / U-O Y / I-K Z / Shift faster / Backspace reset";
	}
 const auto gameplayState = story.CurrentState();
 if (story.enabled && !story.Sandbox()) {
  info.selection="STAGE "+std::to_string(static_cast<int>(gameplayState))+" / "; info.description=story.Objective();
  if (const auto* actor=Selected()) info.selection+=actor->name;
  if (gameplayState!=StoryDirector::GameplayState::PUZZLE) info.controls+=story.BuzzReady() ? " / L fire Buzz's laser / Enter interact" : " / Enter interact / 0 release / N manual";
  if (gameplayState == StoryDirector::GameplayState::PUZZLE) {
   info.puzzleStage=true;
   info.puzzleCode=hallwayPuzzle.CodeDisplay();
   info.description="OBJECTIVE: " + story.Objective();
   const std::string hint = hallwayPuzzle.InteractionHint(penny->Root()->WorldPosition());
   if (!hint.empty()) info.description += " - " + hint;
   info.controls=hallwayPuzzle.KeypadActive()
    ? "0-9 enter / Enter submit / Backspace delete / Esc cancel"
    : "Penny: W/S move / A/D turn / Space stop";
   info.status="Clues found: " + std::to_string(hallwayPuzzle.CluesFound()) + "/3";
   if (statusTimer > 0.0f && !statusText.empty()) info.status += " | " + statusText;
  }
 }
 if (story.enabled && cinematic) { info.selection="THE TOYS ARE SAFE"; info.description="Morning. Everyone escaped the house."; info.controls="The story continues: free exploration next"; }
 info.story=story.enabled;
 info.pennyButton=!arrival.Active() && !cinematic;
 info.pennyControlled=selectedId==pennyId;
 info.pennyLocked=story.enabled && !story.SelectionOpen();
 info.puzzleNotice=statusTimer>0.0f && statusText=="Correct";
 if (info.puzzleNotice) info.status=statusTimer>0.0f ? statusText : hallwayPuzzle.Status();
 if (arrival.Active()) {
  info.story=true; info.arrival=true; info.selection="Night falls on the abandoned house"; info.description=arrival.Caption();
 }
 hud.Render(width,height,info);
}

void ToyRoomApp::OnRender()
{
	renderer.Collect(*scene, camera.position);

	FrameInfo frame;
	frame.camera = &camera;
	frame.aspect = static_cast<float>(width) / static_cast<float>(height);
	frame.lights = &lights;
	frame.ambientLight = environment.AmbientLight();
	frame.clearColor = environment.ClearColor();
	frame.sky = environment.Sky();
	frame.fogColor = environment.HorizonColor();
	frame.sunShadowStrength = outdoorBlend;
	// Night fog over the garden thins away as the morning comes.
	frame.fogDensity=house.exterior->visible ? 0.011f * environment.Night() : 0.0f;
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

	if (hudVisible) DrawHud();
 if (!launch.exportDirectory.empty() && frameCounter==0) ExportScene(launch.exportDirectory);
 if (recording) { RecordFrame(); if (++frameCounter >= launch.frames) Close(); return; }
	if (launch.benchmark > 0) {
		// Skip a warm-up period (shader compilation, driver caches), then time the next N frames.
		constexpr int warmup = 60;
		++frameCounter;
		if (frameCounter == warmup) { glFinish(); benchmarkStart = glfwGetTime(); }
		else if (frameCounter > warmup) {
			benchmarkTriangles += settings.rayTracing ? 2 : renderer.Stats().triangles + renderer.Stats().shadowTriangles;
			benchmarkDraws += settings.rayTracing ? 2 : renderer.Stats().drawCalls + renderer.Stats().shadowDrawCalls;
			if (frameCounter == warmup + launch.benchmark) {
				glFinish();
				const double seconds = glfwGetTime() - benchmarkStart;
				const double n = launch.benchmark;
				std::cout << std::fixed << std::setprecision(2) << "Benchmark: " << launch.benchmark << " frames, "
					<< seconds * 1000.0 / n << " ms/frame (" << n / seconds << " FPS), "
					<< benchmarkDraws / launch.benchmark << " draw calls, "
					<< benchmarkTriangles / launch.benchmark << " triangles per frame" << std::endl;
				// Shapes (scene nodes that carry a mesh) and full-detail triangles, per selectable object.
				std::map<int, std::pair<int, long long>> perOwner;
				for (const DrawItem& item : renderer.Items()) {
					auto& entry = perOwner[item.ownerId];
					++entry.first; entry.second += static_cast<long long>(item.source->TriangleCount());
				}
				for (const auto& [owner, entry] : perOwner)
					std::cout << "  " << (owner >= 0 ? selectables[static_cast<size_t>(owner)].name : std::string("Room and scenery"))
						<< ": " << entry.first << " shapes, " << entry.second << " triangles at full detail" << std::endl;
				Close();
			}
		}
		return;
	}
	// Scripted capture (--capture): read the finished back buffer before it is swapped.
	if (!launch.capture.empty() && ++frameCounter == launch.frames) {
		if (launch.laserDemo) {
			int moved = 0;
			for (const auto& body : physics.Bodies()) if (glm::distance(body.node->local.position, body.initial.position) > 0.08f) ++moved;
			std::cout << "Laser demo displaced " << moved << " / " << physics.Bodies().size() << " blocks\n";
		}
		std::cout << "Penny at "<<penny->Root()->local.position.x<<","<<penny->Root()->local.position.y<<","<<penny->Root()->local.position.z<<" / demo "<<demoPhase<<" leg "<<demoLeg<<"\n";
        std::cout << "Mission capture: " << story.Title() << " / " << environment.ClockText() << "\n";
  for (const auto& c:characters) { const auto p=c->Root()->WorldPosition(); std::cout << c->Name() << " at " << p.x << "," << p.y << "," << p.z << "\n"; }
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
		else if (a == "--no-raytrace") o.noRayTrace = true;
		else if (a == "--lighting") o.lighting=true;
		else if (a == "--textures") o.textures=true;
		else if (a == "--render-settings") o.renderSettings=true;
		else if (a == "--shading" && hasValue) o.shading = std::stoi(argv[++i]);
		else if (a == "--wireframe") o.wireframe = true;
		else if (a == "--normals") o.normals = true;
		else if (a == "--no-hud") o.noHud = true;
		else if (a == "--laser-demo") o.laserDemo = true;
		else if (a == "--guide") o.guide = true;
		else if (a == "--size" && hasValue) {
			const auto v = floats(argv[++i]);
			if (v.size() == 2) { o.windowWidth = std::clamp(static_cast<int>(v[0]), 900, 3840); o.windowHeight = std::clamp(static_cast<int>(v[1]), 600, 2160); }
		}
		else if (a == "--pause") o.pauseClock = true;
		else if (a == "--story") o.story = true;
		else if (a == "--gameplay-demo") o.demo=true;
        else if (a == "--rehearsal-stop") o.rehearsalStop=true;
		else if (a == "--manual") o.manual = true;
		else if (a == "--capture" && hasValue) o.capture = argv[++i];
        else if (a == "--record" && hasValue) o.record = argv[++i];
        else if (a == "--record-fps" && hasValue) o.recordFps = std::clamp(std::stoi(argv[++i]), 20, 60);
        else if (a == "--export" && hasValue) o.exportDirectory = argv[++i];
        else if (a == "--seek" && hasValue) o.seek = std::clamp(std::stof(argv[++i]), 0.0f, 600.0f);
        else if (a == "--haunt") o.haunt = true;
        else if (a == "--no-textures") o.noTextures = true;
        else if (a == "--no-lighting") o.noLighting = true;
        else if (a == "--no-ambient") o.ambient = false;
        else if (a == "--no-diffuse") o.diffuse = false;
        else if (a == "--no-specular") o.specular = false;
        else if (a == "--light-only" && hasValue) o.lightOnly = std::clamp(std::stoi(argv[++i]), 0, MaxLights-1);
        else if (a == "--bounces" && hasValue) o.bounces = std::clamp(std::stoi(argv[++i]), 0, 4);
        else if (a == "--ray-scale" && hasValue) o.rayScale = std::clamp(std::stof(argv[++i]), 0.2f, 1.0f);
        else if (a == "--drive" && hasValue) o.scriptedDrive = std::clamp(std::stof(argv[++i]), -1.0f, 1.0f);
        else if (a == "--turn" && hasValue) o.scriptedTurn = std::clamp(std::stof(argv[++i]), -1.0f, 1.0f);
        else if (a == "--fly" && hasValue) o.scriptedFly = std::clamp(std::stof(argv[++i]), -1.0f, 1.0f);
		else if (a == "--frames" && hasValue) o.frames = std::max(1, std::stoi(argv[++i]));
		else if (a == "--story-steps" && hasValue) o.storySteps=std::clamp(std::stoi(argv[++i]),1,20);
		else if (a == "--intro") o.intro = true;
		else if (a == "--no-intro") o.noIntro = true;
		else if (a == "--benchmark" && hasValue) o.benchmark = std::max(1, std::stoi(argv[++i]));
		else if (a == "--story-step" && hasValue) o.storyStep=std::clamp(std::stof(argv[++i]),0.001f,0.05f);
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
	if (launch.benchmark > 0) glfwSwapInterval(0); // measure the real frame cost, not the monitor rate
	const bool scripted = !launch.capture.empty() || !launch.record.empty();
 if (scripted) { glfwSwapInterval(0); glfwSetWindowAttrib(window,GLFW_RESIZABLE,GLFW_FALSE); }
 // Manual mode and scripted captures without --story use the sandbox: every toy out, every door open.
 if (launch.manual || (scripted && !launch.story)) story.Restart(false);
 if (launch.noIntro || (scripted && !launch.intro)) {
    arrival.Skip();
    const auto savedStep=launch.storyStep,savedDrive=launch.scriptedDrive,savedTurn=launch.scriptedTurn,savedFly=launch.scriptedFly;
    launch.storyStep=launch.scriptedDrive=launch.scriptedTurn=launch.scriptedFly=0;
    StepScene(0);
    launch.storyStep=savedStep;launch.scriptedDrive=savedDrive;launch.scriptedTurn=savedTurn;launch.scriptedFly=savedFly;
 }
	if (launch.hour >= 0.0f) environment.hour = launch.hour;
	if (launch.pauseClock || (scripted && !launch.story)) environment.paused = true;
	if (scripted && !launch.story) { story.enabled=false; story.Pause(); }
	if (launch.manual) { story.enabled=false; story.Pause(); }
	environment.hauntingEnabled = launch.haunt;
	if (launch.rayTrace) settings.rayTracing = true;
	if (launch.noRayTrace) settings.rayTracing = false;
	if (launch.shading >= 0 && launch.shading <= 3) { settings.shadingEnabled=true; settings.shading = static_cast<ShadingMode>(launch.shading); }
	settings.lighting=launch.lighting && !launch.noLighting; settings.textures=launch.textures && !launch.noTextures;
 settings.ambient=launch.ambient; settings.diffuse=launch.diffuse; settings.specular=launch.specular;
 if (launch.bounces>=0) settings.rayBounces=launch.bounces;
 if (launch.rayScale>0) settings.rayScale=launch.rayScale; renderSettingsOpen=launch.renderSettings;
	settings.wireframe = launch.wireframe;
	settings.showNormals = launch.normals;
	hudVisible = !launch.noHud;
	helpVisible = launch.guide;
	if (launch.laserDemo) {
		buzz->Root()->local.position = {4.8f, 0, 0.6f};
		buzz->Root()->local.rotation.y = 0;
		buzz->Special();
		buzz->TiltLaser(12.0f);
		Select(3);
	}
	if (launch.mount) {
		// place Jessie beside Bullseye, then mount through the normal code path
		const float h = glm::radians(bullseye->Heading());
		jessie->Root()->local.position = bullseye->Root()->local.position + glm::vec3(std::cos(h), 0.0f, -std::sin(h)) * 1.2f;
		scene->UpdateWorld(glm::mat4(1.0f));
		Mount();
	}
	if (launch.select >= 0 && launch.select < static_cast<int>(selectables.size()))
		Select(launch.select);
	scene->UpdateWorld(glm::mat4(1.0f)); // arrival.Skip and mounting changed the inspection targets
	if (launch.hasCamera) {
        camera.mode=CameraMode::Free;
		storyCamera=false;
		camera.position = launch.cameraPos;
		camera.position = physics.MoveCamera(camera.position, camera.position);
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
	scene->UpdateWorld(glm::mat4(1.0f));
    if (launch.seek > 0) {
        const float savedStep=launch.storyStep; launch.storyStep=0;
        const auto savedDrive=launch.scriptedDrive,savedTurn=launch.scriptedTurn,savedFly=launch.scriptedFly;
        if (launch.demo) launch.scriptedDrive=launch.scriptedTurn=launch.scriptedFly=0;
        for (float t=0; t<launch.seek; t+=0.05f) StepScene(std::min(0.05f, launch.seek-t));
        launch.storyStep=savedStep;
        launch.scriptedDrive=savedDrive;launch.scriptedTurn=savedTurn;launch.scriptedFly=savedFly;
        if (launch.rehearsalStop) { launch.demo=false;Select(launch.select); }
        if (launch.hasCamera) { camera.mode=CameraMode::Free;camera.position=launch.cameraPos;camera.LookAt(launch.cameraTarget);storyCamera=false; }
    }
    if (!launch.record.empty()) {
        const std::filesystem::path path(launch.record);
        if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
        fopen_s(&recording, launch.record.c_str(), "wb");
        if (!recording) throw std::runtime_error("Cannot open recording stream");
        recordingPixels.resize(static_cast<size_t>(width)*height*3);
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
	if (!settings.rayTracing) {
		const RenderStats& st = renderer.Stats();
		t << " | " << st.drawCalls + st.shadowDrawCalls << " draws, "
		  << (st.triangles + st.shadowTriangles) / 1000 << "k triangles";
	}
	if (settings.rayTracing)
		t << " (" << rayTracer.InstanceCount() << " primitives, x" << settings.rayScale << ")";
	SetTitle(t.str());
}

void ToyRoomApp::PrintHelp() const
{
	std::cout <<
		"\n====================== HAUNTED TOY ROOM ======================\n"
		"MISSION     Enter interact (clues, keypad 257, chest button, doors, wardrobe) / L Buzz laser\n"
		"SELECT      1 Woody  2 Jessie  3 Bullseye  4 Buzz  5 RC Car  6 Ball  7 Lamp  8 Ghost\n"
		"            9 / PENNY button: take Penny over or hand her back to the simulation (any time)\n"
		"            0 nothing   |  left-click any object to select it\n"
		"LIVE        Selecting a toy keeps the other toys running; 0 releases it; N enters full manual\n"
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
		"            F4 RAY TRACING  (-/= resolution, Ctrl+9 bounces)\n"
		"            F5 ambient  F6 diffuse  F7 specular  F8 sun/moon light\n"
		"            F9 normals  F10 vertices  F11 axes gizmo  F12 screenshot (BMP)\n"
		"            V list parts of selected object, Shift+V dump vertex/index tables\n"
		"WORLD       P pause clock  [ ] time speed  , . scrub time  N manual/progression  Shift+N replay\n"
		"            H on-screen guide  G interface  B rebuild blocks  Ctrl+B select block  Esc quit\n"
		"STORY       Outside -> 257 -> toy chest -> Buzz's room -> wardrobe -> laser -> morning\n"
		"            Y skips the opening shot, Shift+N replays the story from the street\n"
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
 if (glGetError()!=GL_NO_ERROR) throw std::runtime_error("OpenGL error before capture");
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
	const std::filesystem::path destination(name);
	if (destination.has_parent_path()) std::filesystem::create_directories(destination.parent_path());
	if (Bmp::Save(name, img))
		std::cout << "Saved " << std::filesystem::absolute(name).string() << "\n";
	else
		std::cout << "Could not save screenshot\n";
}

// CSV inventory is generated from every node, including scenery hidden by cinematic visibility.
void ToyRoomApp::ExportScene(const std::string& directory)
{
    std::filesystem::create_directories(directory);
    std::ofstream out(std::filesystem::path(directory)/"objects.csv");
    out << "path,name,primitive,material,texture_layer,px,py,pz,rx,ry,rz,sx,sy,sz,world_x,world_y,world_z,solid,visible,vertices,triangles,ka,kd,ks,shininess,opacity,reflectivity,unlit,uv_x,uv_y\n";
    std::function<void(const SceneNode&,std::string)> visit = [&](const SceneNode& n,std::string path) {
        const Transform& t=n.local; const auto p=n.WorldPosition();
        const Material* m=n.material;
        out << std::quoted(path) << ',' << std::quoted(n.name) << ',' << (n.mesh?n.mesh->Name():"Joint") << ',' << (m?m->name:"") << ',' << (m?m->rtTextureSlot:-1) << ','
            << t.position.x << ',' << t.position.y << ',' << t.position.z << ',' << t.rotation.x << ',' << t.rotation.y << ',' << t.rotation.z << ','
            << t.scale.x << ',' << t.scale.y << ',' << t.scale.z << ',' << p.x << ',' << p.y << ',' << p.z << ',' << n.solid << ',' << n.visible << ','
            << (n.mesh?n.mesh->Data().vertices.size():0) << ',' << (n.mesh?n.mesh->TriangleCount():0) << ','
            << (m?m->ka:0) << ',' << (m?m->kd:0) << ',' << (m?m->ks:0) << ',' << (m?m->shininess:0) << ',' << (m?m->opacity:0) << ',' << (m?m->reflectivity:0) << ',' << (m?m->unlit:0) << ',' << (m?m->uvScale.x:0) << ',' << (m?m->uvScale.y:0) << '\n';
        int index=0;
        for (const auto& child:n.Children()) visit(*child,path+"/"+child->name+"["+std::to_string(index++)+"]");
    };
    visit(*scene,"World");
    if (!out) throw std::runtime_error("Scene export failed");
    std::ofstream mats(std::filesystem::path(directory)/"materials.csv");
    mats << "name,r,g,b,ka,kd,ks,shininess,emissive_r,emissive_g,emissive_b,opacity,reflectivity,unlit,cutout,texture_layer,uv_x,uv_y\n";
    std::set<const Material*> seen;
    scene->ForEach([&](SceneNode& n) {
        if (!n.material || !seen.insert(n.material).second) return;
        const auto& m=*n.material;
        mats << m.name << ',' << m.color.r << ',' << m.color.g << ',' << m.color.b << ',' << m.ka << ',' << m.kd << ',' << m.ks << ',' << m.shininess << ',' << m.emissive.r << ',' << m.emissive.g << ',' << m.emissive.b << ',' << m.opacity << ',' << m.reflectivity << ',' << m.unlit << ',' << m.cutout << ',' << m.rtTextureSlot << ',' << m.uvScale.x << ',' << m.uvScale.y << '\n';
    });
    std::ofstream lamp(std::filesystem::path(directory)/"lights.csv");
    lamp << "slot,name,type,enabled,shadows,intensity,r,g,b,kc,kl,kq,inner_degrees,outer_degrees\n";
    for (size_t i=0;i<lights.size();++i) {
        const auto& l=lights[i];
        lamp << i << ',' << l.name << ',' << static_cast<int>(l.type) << ',' << l.enabled << ',' << l.castsShadows << ',' << l.intensity << ',' << l.color.r << ',' << l.color.g << ',' << l.color.b << ',' << l.constant << ',' << l.linear << ',' << l.quadratic << ',' << glm::degrees(std::acos(l.innerCutoff)) << ',' << glm::degrees(std::acos(l.outerCutoff)) << '\n';
    }
    assets.ExportTextures((std::filesystem::path(directory)/"textures").string());
    std::cout << "Exported scene inventory to " << directory << '\n';
}

void ToyRoomApp::RecordFrame()
{
    const size_t bytes=static_cast<size_t>(width)*height*3;
    recordingPixels.resize(bytes);
    glPixelStorei(GL_PACK_ALIGNMENT,1); glReadBuffer(GL_BACK);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,recordingPixels.data());
    if (std::fwrite(recordingPixels.data(),1,bytes,recording)!=bytes) throw std::runtime_error("Recording write failed");
}


void ToyRoomApp::DemoMission(float dt)
{
    // Deterministic rehearsal of the whole story. It drives only Penny, with the same movement,
    // Enter interactions, keypad and L action as a player; the story layer moves everyone else.
    // It never sets actor positions or advances states without their real conditions.
    using namespace RoomSize;
    demoWait+=dt;
    auto select=[&](int id) { selectedId=id; story.SetControlled(DrivenCharacter(),jessieMounted ? jessie : nullptr); physics.SetIndependentActor(DrivenCharacter()->Root()); };
    auto next=[&]() { ++demoPhase;demoLeg=0;demoWait=0;std::cout<<"DEMO phase "<<demoPhase<<" time "<<time<<"\n"; };
    auto move=[&](Character* actor,std::initializer_list<glm::vec3> goals) {
        const auto points=goals;
        if (demoLeg>=static_cast<int>(points.size())) return true;
        const glm::vec3 goal=*(points.begin()+demoLeg), at=actor->Root()->local.position;
        if (actor->FollowWaypoint(goal,dt,2.5f) || glm::length(glm::vec2(goal.x-at.x,goal.z-at.z))<0.3f) ++demoLeg;
        return demoLeg>=static_cast<int>(points.size());
    };
    auto interact=[&]() { statusText=story.Interact(penny.get()); statusTimer=4; if (!statusText.empty()) std::cout<<"INTERACT "<<statusText<<"\n"; };
    Cat* cat=penny.get();
    const float porch=Ground+0.45f;
    switch (demoPhase) {
    // 1. Night outside: along the pavement, through the gate, up the porch and in at the open door.
    case 0: select(pennyId); if (move(cat,{{11.4f,Ground,23.2f},{12,Ground,20.6f},{12,Ground,14.6f},{12,porch,12.2f},{12,porch,9.9f},{12,Ground,7.4f}})) next(); break;
    case 1: if (story.CurrentState()==StoryDirector::GameplayState::PUZZLE) next(); break;
    // 2. Upstairs: the corridor, the landing and the flight of stairs into the hallway; three clues.
    case 2: if (move(cat,{{12,Ground,-7.6f},{15,Ground,-8.0f},{15,Ground,-7.2f},{15,0,1.6f},{15,0,4.4f}})) { hallwayPuzzle.Interact(cat->Root()->WorldPosition());next(); } break;
    case 3: if (move(cat,{{13.3f,0,2.5f}})) { hallwayPuzzle.Interact(cat->Root()->WorldPosition());next(); } break;
    case 4: if (move(cat,{{11.8f,0,3.1f}})) { hallwayPuzzle.Interact(cat->Root()->WorldPosition());next(); } break;
    case 5: if (move(cat,{{12.3f,0,5.5f}})) {
        hallwayPuzzle.Interact(cat->Root()->WorldPosition());
        if (!hallwayPuzzle.KeypadActive()) break;
        for(int digit:{0,0,0}) hallwayPuzzle.AddDigit(digit);
        if (hallwayPuzzle.Submit()!=PuzzleCode::Result::Incorrect) throw std::runtime_error("Wrong code unlocked the door");
        for(int digit:{2,5,7}) hallwayPuzzle.AddDigit(digit);
        if (hallwayPuzzle.Submit()!=PuzzleCode::Result::Correct) throw std::runtime_error("Valid puzzle failed");
        story.Advance(StoryDirector::Transition::PUZZLE_SOLVED);next();
    } break;
    // 3. The toy chest: press the red button, then watch the toys come alive.
    case 6: if (demoWait>0.6f && move(cat,{{11,0,4},{8,0,4},{3.4f,0,4.9f},{1.4f,0,3.6f}})) { interact();next(); } break;
    case 7: if (story.CurrentState()==StoryDirector::GameplayState::BUZZ_ROOM) next(); break;
    // 4-5. Leave together, down the stairs, and open the ordinary door on the corridor's left.
    case 8: if (move(cat,{{3.4f,0,4.9f},{8,0,4},{12,0,4},{15,0,2.6f},{15,0,0.8f},{15,Ground,-7.4f},{15,Ground,-8.1f},{12,Ground,-7.6f},{11.9f,Ground,-5.8f}})) { interact();next(); } break;
    case 9: if (story.CurrentState()==StoryDirector::GameplayState::WARDROBE && demoWait>1.2f) next(); break;
    // 6. The wardrobe: too high for Penny. Jessie and Bullseye do the rest.
    // Penny must stand right in front of the doors to try them, then steps back to make room for Bullseye.
    case 10: if (move(cat,{{9.4f,Ground,-6.1f},{6.4f,Ground,-6.4f},{4.7f,Ground,-6.0f}})) { interact();next(); } break;
    case 11: if (story.CurrentState()==StoryDirector::GameplayState::FINAL_ESCAPE) next(); else move(cat,{{6.6f,Ground,-7.6f}}); break;
    // 7-8. Back to the entrance hall. The main door is locked.
    case 12: if (demoWait>1.5f && move(cat,{{9.4f,Ground,-6.0f},{12,Ground,-5.6f},{12,Ground,0},{12,Ground,7.6f}})) { interact();next(); } break;
    // Beside the door, clear of the toys' waiting places and below Buzz's line of fire.
    case 13: if (move(cat,{{11.7f,Ground,7.3f}})) next(); break;
    // 9. Buzz flies into position; the player's L fires the laser and the door breaks apart.
    case 14: if (story.BuzzReady() && demoWait>1.0f) { if (story.RequestLaser()) std::cout<<"DEMO L laser\n"; next(); } break;
    case 15: if (story.DoorBroken() && demoWait>1.2f) next(); break;
    // 10. Outside into the morning; the camera pulls back, then free exploration begins.
    case 16: if (move(cat,{{12,Ground,8.4f},{12,porch,10.4f},{12,porch,12.0f},{12,Ground,14.2f},{12.8f,Ground,16.2f}})) next(); break;
    case 17: if (story.CurrentState()==StoryDirector::GameplayState::FREE_EXPLORE) next(); break;
    default: break;
    }
}
