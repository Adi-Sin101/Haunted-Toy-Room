#include "House.h"

#include <cmath>
#include <string>

#include <glm/gtc/constants.hpp>

#include "Room.h"
#include "render/Assets.h"
#include "scene/SceneNode.h"

namespace {

using namespace RoomSize;

// A material whose greyscale detail texture (siding, shingles, brick, grass, wood) is tinted by `color`.
// `repeat` = how many times the texture repeats across a face. Both renderers use the same image.
Material& Tinted(Assets& a, const std::string& name, const glm::vec3& color, const char* texture,
	const glm::vec2& repeat, float ks = 0.1f, float shininess = 16.0f)
{
	Material& m = a.Mat(name, color, ks, shininess);
	m.texture = a.Named(texture);
	m.rtTextureSlot = a.NamedSlot(texture);
	m.uvScale = repeat;
	return m;
}

// A box given by its two opposite corners (lo, hi) instead of centre and size.
SceneNode* Box(SceneNode& parent, const char* name, const Mesh& cube, const Material& mat, const glm::vec3& lo, const glm::vec3& hi)
{
	return parent.AddShape(name, &cube, &mat, (lo + hi) * 0.5f, hi - lo);
}

// A lap-sided wall box whose siding boards keep the same real size on every wall (one texture repeat
// = 8 boards = 2.4 units). Each wall gets its own material because the repeat count depends on its size.
SceneNode* SidedBox(SceneNode& parent, Assets& a, const std::string& name, const glm::vec3& color,
	const glm::vec3& lo, const glm::vec3& hi)
{
	const glm::vec3 size = hi - lo;
	const float across = std::max(size.x, size.z);
	Material& m = Tinted(a, "siding-" + name, color, "siding", {across / 2.4f, size.y / 2.4f}, 0.08f, 12.0f);
	return Box(parent, name.c_str(), a.Cube(), m, lo, hi);
}

// A triangular gable made from ONE cube: a unit cube turned 45 degrees about its thickness axis is a
// diamond with corners at distance 0.707; sizing it by sqrt(2) puts the corners at 1, and the parent
// joint's non-uniform scale stretches the diamond to half-width `halfSpan` and half-height `rise`.
// The lower half of the diamond lies inside the wall below and is never seen, leaving a triangle.
//   alongX = false: the gable faces +-X (thickness along X, triangle in the YZ plane)
//   alongX = true : the gable faces +-Z (thickness along Z, triangle in the XY plane)
void Gable(SceneNode& parent, const char* name, const Mesh& cube, const Material& mat, const glm::vec3& base,
	float halfSpan, float rise, float thickness, bool alongX)
{
	SceneNode* joint = parent.AddChild(name);
	joint->local.position = base;
	const float r2 = glm::root_two<float>();
	if (alongX) {
		joint->local.scale = {halfSpan, rise, 1.0f};
		joint->AddShape(name, &cube, &mat, {0, 0, 0}, {r2, r2, thickness}, {0, 0, 45});
	}
	else {
		joint->local.scale = {1.0f, rise, halfSpan};
		joint->AddShape(name, &cube, &mat, {0, 0, 0}, {thickness, r2, r2}, {45, 0, 0});
	}
}

// A decorative window on an exterior wall facing +Z (sign = 1) or -Z (sign = -1): frame, glass, mullions.
void Window(SceneNode& parent, Assets& a, const glm::vec3& centre, float w, float h, float sign = 1.0f)
{
	const Mesh& cube = a.Cube();
	Material& frame = a.Mat("house-trim", {0.95f, 0.95f, 0.93f}, 0.25f, 32.0f);
	// The pane's texture holds the glass, the inner frame and the cross mullions (was 3 more cubes + a sill).
	Material& pane = a.Mat("house-window-pane", glm::vec3(1.0f), 0.9f, 96.0f);
	pane.texture = a.Named("window-pane");
	pane.plainColor = {0.22f, 0.32f, 0.45f};
	pane.reflectivity = 0.2f;
	parent.AddShape("WindowFrame", &cube, &frame, centre + glm::vec3(0, 0, 0.06f * sign), {w + 0.3f, h + 0.3f, 0.1f});
	parent.AddShape("WindowPane", &cube, &pane, centre + glm::vec3(0, 0, 0.12f * sign), {w, h, 0.04f});
}

// A picket-fence / railing panel: ONE thin box whose texture has transparent gaps (alpha cut-out), in
// place of a row of picket cubes and rails. `spacing` = distance between pickets (8 per texture repeat).
void CutoutPanel(SceneNode& parent, Assets& a, const std::string& name, const glm::vec3& lo, const glm::vec3& hi, float spacing)
{
	Material& m = a.Mat("cutout-" + name, {0.97f, 0.97f, 0.95f}, 0.2f, 24.0f);
	m.texture = a.SlotTexture(Assets::PicketSlot);
	m.rtTextureSlot = Assets::PicketSlot;
	m.cutout = true;
	m.uvScale = {(hi.x - lo.x) / (8.0f * spacing), 1.0f};
	Box(parent, name.c_str(), a.Cube(), m, lo, hi);
}

// A tree: trunk cylinder and three overlapping foliage spheres (drawn with the low-detail mesh when far).
void Tree(SceneNode& parent, Assets& a, const glm::vec3& foot, float height)
{
	Material& bark = a.Mat("tree-bark", {0.36f, 0.25f, 0.16f}, 0.05f);
	Material& leaves = a.Mat("tree-leaves", {0.24f, 0.48f, 0.20f}, 0.05f);
	Material& leavesLight = a.Mat("tree-leaves-light", {0.36f, 0.60f, 0.24f}, 0.05f);
	SceneNode* tree = parent.AddChild("Tree");
	tree->local.position = foot;
	tree->AddShape("Trunk", &a.Cylinder(), &bark, {0, height * 0.3f, 0}, {0.6f, height * 0.6f, 0.6f});
	tree->AddShape("Foliage", &a.Sphere(), &leaves, {0, height * 0.72f, 0}, glm::vec3(height * 0.55f));
	tree->AddShape("Foliage", &a.Sphere(), &leavesLight, {height * 0.2f, height * 0.88f, height * 0.12f}, glm::vec3(height * 0.40f));
	tree->AddShape("Foliage", &a.Sphere(), &leaves, {-height * 0.22f, height * 0.62f, -height * 0.1f}, glm::vec3(height * 0.42f));
}

} // namespace

HouseRig BuildHouse(SceneNode& root, Assets& a)
{
	HouseRig rig;
	const Mesh& plane = a.Plane();
	const Mesh& cube = a.Cube();
	const Mesh& sphere = a.Sphere();
	const Mesh& cylinder = a.Cylinder();

	// =========================================================================================
	// Interior: corridor, landing, stairs, stairwell (always present, it is inside the house)
	// =========================================================================================
	SceneNode* inside = root.AddChild("HouseInterior");
	rig.interior = inside;
	Material& woodFloor = a.Mat("ground-floor-wood", {0.92f, 0.82f, 0.70f}, 0.3f, 32.0f);
	woodFloor.texture = a.SlotTexture(Assets::FloorSlot); woodFloor.rtTextureSlot = Assets::FloorSlot; woodFloor.uvScale = {1.5f, 6.0f};
	Material& hallWall = a.Mat("ground-floor-wall", {0.86f, 0.80f, 0.68f}, 0.05f, 8.0f);
	Material& ceiling = a.Mat("ceiling", {0.85f, 0.83f, 0.78f}, 0.0f);
	const float floorY = Ground + 0.02f; // just above the lawn, which continues under the house
	const float cz = (StairBottomZ - 9.05f) * 0.5f;

	// Corridor (front door -> back of the house) and the stair landing at its far end.
	inside->AddShape("CorridorFloor", &plane, &woodFloor, {(CorridorLeft + CorridorRight) * 0.5f, floorY, 0}, {CorridorRight - CorridorLeft, 1, 18.1f});
	inside->AddShape("LandingFloor", &plane, &woodFloor, {(StairLeft + StairRight) * 0.5f, floorY, cz}, {StairRight - StairLeft, 1, StairBottomZ + 9.05f});
	inside->AddShape("CorridorCeiling", &plane, &ceiling, {(CorridorLeft + CorridorRight) * 0.5f, Slab, 0}, {CorridorRight - CorridorLeft, 1, 18.1f}, {180, 0, 0});
	Material& runner = a.Mat("corridor-runner", {0.55f, 0.20f, 0.18f}, 0.0f);
	runner.texture = a.SlotTexture(Assets::RugSlot); runner.rtTextureSlot = Assets::RugSlot; runner.uvScale = {1, 4};
	inside->AddShape("CorridorRunner", &plane, &runner, {(CorridorLeft + CorridorRight) * 0.5f, floorY + 0.01f, 1.0f}, {1.6f, 1, 14.0f});

	// Walls are one-sided planes facing into the space (rotation (90, yaw, 0): yaw 90 faces +X, -90 faces -X, 0 faces +Z).
	const float h = Slab - Ground;
	auto wall = [&](const char* name, const glm::vec3& centre, float yaw, float width, float height) {
		inside->AddShape(name, &plane, &hallWall, centre, {width, 1, height}, {90, yaw, 0});
	};
	wall("CorridorLeftWall", {CorridorLeft, Ground + h * 0.5f, 0}, 90, 18.1f, h);
	wall("CorridorRightWall", {CorridorRight, Ground + h * 0.5f, (StairBottomZ + 9.05f) * 0.5f}, -90, 9.05f - StairBottomZ, h);
	const float well = DoorHeight - Ground;                  // stairwell: ground floor up to the hall ceiling
	wall("StairwellEnd", {(CorridorLeft + StairRight) * 0.5f, Ground + well * 0.5f, -9.05f}, 0, StairRight - CorridorLeft, well);
	wall("StairwellLeft", {StairLeft, Ground + well * 0.5f, (StairBottomZ + StairTopZ) * 0.5f}, 90, StairTopZ - StairBottomZ, well);
	wall("StairwellLeftUpper", {StairLeft, (Slab + DoorHeight) * 0.5f, cz}, 90, StairBottomZ + 9.05f, DoorHeight - Slab);
	wall("StairwellRight", {StairRight, Ground + well * 0.5f, (StairTopZ - 9.05f) * 0.5f}, -90, StairTopZ + 9.05f, well);
	inside->AddShape("StairwellCeiling", &plane, &ceiling, {(StairLeft + StairRight) * 0.5f, DoorHeight, (StairTopZ - 9.05f) * 0.5f},
		{StairRight - StairLeft, 1, StairTopZ + 9.05f}, {180, 0, 0});

	// The flight: 18 solid steps, rise 0.25 and tread 8/18, from the landing (y = Ground) to the hallway (y = 0).
	Material& treads = a.Mat("stair-wood", {0.80f, 0.62f, 0.46f}, 0.3f, 32.0f);
	treads.texture = a.SlotTexture(Assets::FloorSlot); treads.rtTextureSlot = Assets::FloorSlot; treads.uvScale = {0.6f, 0.2f};
	constexpr int steps = 18;
	const float rise = -Ground / steps, tread = (StairTopZ - StairBottomZ) / steps;
	for (int i = 0; i < steps; ++i) {
		const float top = Ground + rise * static_cast<float>(i + 1);
		const float z0 = StairBottomZ + tread * static_cast<float>(i);
		// inset 2 cm from the side walls so the step faces never coincide with the wall planes (z-fighting)
		Box(*inside, "Step", cube, treads, {StairLeft + 0.02f, Ground, z0}, {StairRight - 0.02f, top, z0 + tread});
	}
	// Handrail along the slope: a cylinder (axis = local Y) tipped forward by 90 - atan(rise / run).
	Material& brass = a.Mat("brass", {0.85f, 0.65f, 0.25f}, 0.8f, 64.0f);
	const float run = StairTopZ - StairBottomZ, climb = -Ground;
	const float railLength = std::sqrt(run * run + climb * climb);
	const float tilt = glm::degrees(std::atan2(run, climb));
	inside->AddShape("Handrail", &cylinder, &brass, {StairRight - 0.2f, Ground + climb * 0.5f + 1.0f, (StairBottomZ + StairTopZ) * 0.5f},
		{0.08f, railLength, 0.08f}, {tilt, 0, 0});
	Material& lampGlow = a.Mat("hall-ceiling-lamp", glm::vec3(0.0f), 0.0f);
	lampGlow.unlit = true; lampGlow.emissive = {1.0f, 0.92f, 0.75f};
	inside->AddShape("CorridorLamp", &sphere, &lampGlow, {(CorridorLeft + CorridorRight) * 0.5f, Slab - 0.12f, 2.0f}, {0.5f, 0.2f, 0.5f});

	// Toy-room double door: two leaves hinged at the doorway's jambs, opening into the hallway.
	Material& doorWood = a.Mat("door-wood", {0.80f, 0.64f, 0.48f}, 0.25f, 24.0f);
	doorWood.texture = a.SlotTexture(Assets::FloorSlot); doorWood.rtTextureSlot = Assets::FloorSlot; doorWood.uvScale = {0.5f, 1.0f};
	Material& doorPanel = a.Mat("door-panel", {0.70f, 0.54f, 0.40f}, 0.2f, 24.0f);
	const float leaf = (DoorHigh - DoorLow) * 0.5f;
	for (int side = 0; side < 2; ++side) {
		const float dir = side == 0 ? 1.0f : -1.0f;     // left leaf extends +Z from its hinge, right leaf -Z
		SceneNode* hinge = root.AddChild(side == 0 ? "RoomDoorLeft" : "RoomDoorRight");
		hinge->local.position = {HalfWidth + 0.05f, 0, side == 0 ? DoorLow : DoorHigh};
		hinge->AddShape("DoorLeaf", &cube, &doorWood, {0, (DoorHeight - 0.1f) * 0.5f, dir * leaf * 0.5f}, {0.08f, DoorHeight - 0.1f, leaf - 0.04f});
		for (float y : {1.25f, 3.25f}) hinge->AddShape("DoorPanel", &cube, &doorPanel, {0, y, dir * leaf * 0.5f}, {0.12f, 1.5f, leaf - 0.7f});
		hinge->AddShape("DoorKnob", &sphere, &brass, {0, 2.2f, dir * (leaf - 0.3f)}, {0.2f, 0.14f, 0.14f});
		(side == 0 ? rig.roomDoorLeft : rig.roomDoorRight) = hinge;
	}
	// Stair-head door: fills the opening in the hall's z = DoorLow wall. Once it is shut nothing below
	// the upper floor can be seen, so the whole interior node is hidden (no overdraw, fewer ray tests).
	rig.stairDoor = root.AddChild("StairDoor");
	rig.stairDoor->local.position = {StairLeft, 0, DoorLow};
	const float stairDoorWidth = StairRight - StairLeft;
	rig.stairDoor->AddShape("DoorLeaf", &cube, &doorWood, {stairDoorWidth * 0.5f, (DoorHeight - 0.1f) * 0.5f, 0}, {stairDoorWidth - 0.04f, DoorHeight - 0.1f, 0.1f});
	for (float y : {1.25f, 3.25f}) rig.stairDoor->AddShape("DoorPanel", &cube, &doorPanel, {stairDoorWidth * 0.5f, y, 0}, {stairDoorWidth - 0.7f, 1.5f, 0.14f});
	rig.stairDoor->AddShape("DoorKnob", &sphere, &brass, {stairDoorWidth - 0.3f, 2.2f, 0}, {0.14f, 0.14f, 0.24f});

	// =========================================================================================
	// Exterior (hidden once Penny is inside: none of it can be seen from the rooms)
	// =========================================================================================
	SceneNode* ext = root.AddChild("HouseExterior");
	rig.exterior = ext;
	const glm::vec3 yellow(0.96f, 0.80f, 0.42f), garageYellow(0.90f, 0.76f, 0.45f);
	Material& trim = a.Mat("house-trim", {0.95f, 0.95f, 0.93f}, 0.25f, 32.0f);
	Material& brick = Tinted(a, "house-brick", {0.68f, 0.32f, 0.24f}, "brick", {1, 1.5f}, 0.05f, 8.0f);
	const float top = Height;                                        // the walls reach the toy room's ceiling

	// Walls (cubes 0.3 thick, outside the room's one-sided walls)
	SidedBox(*ext, a, "front-left", yellow, {HouseLeft, Ground, 9.05f}, {FrontDoorLeft, top, HouseFront});
	SidedBox(*ext, a, "front-right", yellow, {FrontDoorRight, Ground, 9.05f}, {HouseRight, top, HouseFront});
	SidedBox(*ext, a, "front-over-door", yellow, {FrontDoorLeft, Ground + FrontDoorHeight, 9.05f}, {FrontDoorRight, top, HouseFront});
	SidedBox(*ext, a, "back-left", yellow, {HouseLeft, Ground, HouseBack}, {0.5f, top, -9.05f});
	SidedBox(*ext, a, "back-right", yellow, {4.5f, Ground, HouseBack}, {HouseRight, top, -9.05f});
	SidedBox(*ext, a, "back-under-window", yellow, {0.5f, Ground, HouseBack}, {4.5f, 2.5f, -9.05f});
	SidedBox(*ext, a, "back-over-window", yellow, {0.5f, 5.5f, HouseBack}, {4.5f, top, -9.05f});
	SidedBox(*ext, a, "left", yellow, {HouseLeft, Ground, -9.05f}, {-10.05f, top, 9.05f});
	SidedBox(*ext, a, "right", yellow, {17.1f, Ground, -9.05f}, {HouseRight, top, 9.05f});
	// White trim: corner boards and a band at the upper floor's level
	for (float x : {HouseLeft - 0.05f, HouseRight + 0.05f}) for (float z : {HouseBack - 0.05f, HouseFront + 0.05f})
		ext->AddShape("CornerBoard", &cube, &trim, {x, (Ground + top) * 0.5f, z}, {0.3f, top - Ground, 0.3f});
	Box(*ext, "FloorBand", cube, trim, {HouseLeft, Slab - 0.25f, HouseFront}, {HouseRight, Slab + 0.05f, HouseFront + 0.08f});
	Box(*ext, "Foundation", cube, brick, {HouseLeft - 0.1f, Ground - 0.05f, HouseBack - 0.1f}, {HouseRight + 0.1f, Ground + 0.35f, HouseFront + 0.1f});

	// Roof: two shingle slabs meeting at a ridge along X, overhanging the walls by 0.8.
	Material& shingles = Tinted(a, "roof-shingles", {0.86f, 0.42f, 0.28f}, "shingles", {12, 5}, 0.12f, 12.0f);
	const float eaveY = top - 0.4f, ridgeY = top + 5.0f, eaveZ = HouseFront + 0.8f;
	const float slope = std::atan2(ridgeY - eaveY, eaveZ);
	const float slab = std::sqrt(eaveZ * eaveZ + (ridgeY - eaveY) * (ridgeY - eaveY));
	const float roofLength = HouseRight - HouseLeft + 1.6f, roofX = (HouseLeft + HouseRight) * 0.5f;
	for (float side : {1.0f, -1.0f})
		ext->AddShape("RoofSlab", &cube, &shingles, {roofX, (eaveY + ridgeY) * 0.5f + 0.12f, side * eaveZ * 0.5f},
			{roofLength, 0.25f, slab}, {side * glm::degrees(slope), 0, 0});
	ext->AddShape("Ridge", &cube, &shingles, {roofX, ridgeY + 0.2f, 0}, {roofLength, 0.25f, 0.5f});
	Material& gableSiding = Tinted(a, "siding-gable", yellow, "siding", {8, 2}, 0.08f, 12.0f);
	Gable(*ext, "GableLeft", cube, gableSiding, {HouseLeft + 0.2f, top, 0}, 9.35f, 5.0f, 0.36f, false);
	Gable(*ext, "GableRight", cube, gableSiding, {HouseRight - 0.18f, top, 0}, 9.35f, 5.0f, 0.3f, false);
	// Fascia boards under the eaves and the chimney
	for (float side : {1.0f, -1.0f})
		ext->AddShape("Fascia", &cube, &trim, {roofX, eaveY - 0.15f, side * eaveZ}, {roofLength, 0.3f, 0.12f});
	Box(*ext, "Chimney", cube, brick, {-6.6f, top, -4.6f}, {-5.2f, ridgeY + 2.2f, -3.2f});
	Box(*ext, "ChimneyCap", cube, a.Mat("chimney-cap", {0.25f, 0.24f, 0.24f}, 0.1f), {-6.8f, ridgeY + 2.2f, -4.8f}, {-5.0f, ridgeY + 2.5f, -3.0f});
	// Attic window high in the left gable and the upstairs front windows
	Window(*ext, a, {-1.0f, 2.6f + 1.4f, HouseFront}, 2.0f, 2.2f);
	Window(*ext, a, {-6.5f, 2.6f + 1.4f, HouseFront}, 2.0f, 2.2f);
	Window(*ext, a, {4.5f, 2.6f + 1.4f, HouseFront}, 2.0f, 2.2f);

	// Ground-floor windows (one with a flower box, as in the reference house)
	Window(*ext, a, {-7.0f, Ground + 2.2f, HouseFront}, 2.0f, 1.9f);
	Window(*ext, a, {-2.5f, Ground + 2.2f, HouseFront}, 2.0f, 1.9f);
	Window(*ext, a, {3.2f, Ground + 2.2f, HouseFront}, 3.4f, 1.9f);
	Material& planter = a.Mat("flower-box", {0.92f, 0.92f, 0.90f}, 0.2f);
	Box(*ext, "FlowerBox", cube, planter, {1.4f, Ground + 0.75f, HouseFront + 0.1f}, {5.0f, Ground + 1.15f, HouseFront + 0.55f});
	Material& flowers = a.Mat("flower-bed", glm::vec3(1.0f), 0.05f, 8.0f);
	flowers.texture = a.Named("flower-bed");
	flowers.plainColor = {0.32f, 0.55f, 0.28f};
	flowers.uvScale = {3.0f, 1.0f};
	Box(*ext, "FlowerBed", cube, flowers, {1.5f, Ground + 1.15f, HouseFront + 0.15f}, {4.9f, Ground + 1.42f, HouseFront + 0.5f});

	// Front door (hinged on its left jamb, opens inward) with a small window and a knob
	Material& doorRed = a.Mat("front-door", {0.70f, 0.50f, 0.38f}, 0.25f, 24.0f);
	doorRed.texture = a.SlotTexture(Assets::FloorSlot); doorRed.rtTextureSlot = Assets::FloorSlot; doorRed.uvScale = {0.4f, 1.0f};
	rig.frontDoor = root.AddChild("FrontDoor");
	rig.frontDoor->local.position = {FrontDoorLeft, Ground, 9.2f};
	const float doorWidth = FrontDoorRight - FrontDoorLeft;
	rig.frontDoor->AddShape("FrontDoorLeaf", &cube, &doorRed, {doorWidth * 0.5f, FrontDoorHeight * 0.5f, 0}, {doorWidth - 0.04f, FrontDoorHeight - 0.04f, 0.1f});
	rig.frontDoor->AddShape("FrontDoorWindow", &cube, &a.Mat("house-window-glass", {0.22f, 0.32f, 0.45f}, 0.9f, 96.0f),
		{doorWidth * 0.5f, FrontDoorHeight * 0.72f, 0.04f}, {0.6f, 0.6f, 0.06f});
	rig.frontDoor->AddShape("FrontDoorKnob", &sphere, &brass, {doorWidth - 0.2f, FrontDoorHeight * 0.45f, 0.08f}, glm::vec3(0.12f));
	for (float x : {FrontDoorLeft - 0.12f, FrontDoorRight + 0.12f})
		ext->AddShape("DoorCasing", &cube, &trim, {x, Ground + FrontDoorHeight * 0.5f, HouseFront + 0.04f}, {0.2f, FrontDoorHeight, 0.12f});
	ext->AddShape("DoorCasing", &cube, &trim, {12.0f, Ground + FrontDoorHeight + 0.1f, HouseFront + 0.04f}, {doorWidth + 0.44f, 0.2f, 0.12f});

	// Porch: deck, steps, brick column bases with white columns, railing and its own gable roof
	const float porchTop = Ground + 0.45f, porchFront = 12.5f;
	Material& deck = a.Mat("porch-deck", {0.62f, 0.40f, 0.28f}, 0.2f, 16.0f);
	deck.texture = a.SlotTexture(Assets::FloorSlot); deck.rtTextureSlot = Assets::FloorSlot; deck.uvScale = {2.0f, 1.0f};
	Box(*ext, "PorchDeck", cube, deck, {8.0f, Ground, HouseFront}, {16.0f, porchTop, porchFront});
	for (int i = 0; i < 3; ++i) {
		const float z0 = porchFront + i * 0.4f;
		Box(*ext, "PorchStep", cube, deck, {10.8f, Ground, z0}, {13.2f, porchTop - 0.15f * static_cast<float>(i + 1) + 0.15f, z0 + 0.4f});
	}
	const float porchRoofY = Slab + 0.1f;
	for (float x : {8.35f, 15.65f}) {
		Box(*ext, "ColumnBase", cube, brick, {x - 0.35f, porchTop, porchFront - 0.75f}, {x + 0.35f, porchTop + 1.3f, porchFront - 0.05f});
		Box(*ext, "Column", cube, trim, {x - 0.17f, porchTop + 1.3f, porchFront - 0.57f}, {x + 0.17f, porchRoofY - 1.4f, porchFront - 0.23f});
	}
	CutoutPanel(*ext, a, "railing-left", {8.7f, porchTop, porchFront - 0.43f}, {10.7f, porchTop + 0.95f, porchFront - 0.37f}, 0.4f);
	CutoutPanel(*ext, a, "railing-right", {13.3f, porchTop, porchFront - 0.43f}, {15.3f, porchTop + 0.95f, porchFront - 0.37f}, 0.4f);
	const float porchHalf = 4.6f, porchRise = 1.4f, porchDepth = porchFront + 0.5f - HouseFront;
	const float porchSlope = std::atan2(porchRise, porchHalf), porchSlab = std::sqrt(porchHalf * porchHalf + porchRise * porchRise);
	Material& porchShingles = Tinted(a, "porch-shingles", {0.86f, 0.42f, 0.28f}, "shingles", {3, 2}, 0.12f, 12.0f);
	for (float side : {-1.0f, 1.0f})
		ext->AddShape("PorchRoof", &cube, &porchShingles, {12.0f + side * porchHalf * 0.5f, porchRoofY + porchRise * 0.5f + 0.12f, HouseFront + porchDepth * 0.5f},
			{porchSlab, 0.22f, porchDepth}, {0, 0, -side * glm::degrees(porchSlope)});
	Material& porchGable = Tinted(a, "siding-porch-gable", yellow, "siding", {3, 1}, 0.08f, 12.0f);
	// The frieze beam encloses the lower half of the gable's diamond (there is no wall below it here).
	Gable(*ext, "PorchGable", cube, porchGable, {12.0f, porchRoofY, porchFront + 0.35f}, porchHalf - 0.3f, porchRise - 0.1f, 0.2f, true);
	Box(*ext, "PorchFrieze", cube, trim, {7.6f, porchRoofY - 1.4f, porchFront + 0.1f}, {16.4f, porchRoofY, porchFront + 0.6f});
	Box(*ext, "PorchBeam", cube, trim, {7.6f, porchRoofY - 1.4f, HouseFront}, {16.4f, porchRoofY - 1.1f, porchFront + 0.1f});
	ext->AddShape("Planter", &cylinder, &a.Mat("planter-barrel", {0.55f, 0.38f, 0.24f}, 0.1f), {14.4f, porchTop + 0.4f, HouseFront + 0.8f}, {0.8f, 0.8f, 0.8f});
	ext->AddShape("PorchPlant", &sphere, &a.Mat("tree-leaves-light", {0.36f, 0.60f, 0.24f}, 0.05f), {14.4f, porchTop + 1.2f, HouseFront + 0.8f}, {1.1f, 0.9f, 1.1f});
	Material& porchLight = a.Mat("porch-lantern", glm::vec3(0.1f), 0.0f);
	porchLight.unlit = true; porchLight.emissive = {1.0f, 0.85f, 0.55f};
	ext->AddShape("PorchLantern", &cube, &porchLight, {10.5f, Ground + 2.6f, HouseFront + 0.15f}, {0.22f, 0.4f, 0.22f});

	// Garage on the left with its own small gable roof
	const float gx0 = -19.0f, gx1 = HouseLeft, gz0 = -5.0f, gz1 = 7.5f, gTop = Ground + 4.5f;
	SidedBox(*ext, a, "garage-front", garageYellow, {gx0, Ground, gz1 - 0.3f}, {gx1, gTop, gz1});
	SidedBox(*ext, a, "garage-side", garageYellow, {gx0, Ground, gz0}, {gx0 + 0.3f, gTop, gz1});
	SidedBox(*ext, a, "garage-back", garageYellow, {gx0, Ground, gz0}, {gx1, gTop, gz0 + 0.3f});
	Box(*ext, "GarageDoor", cube, Tinted(a, "garage-door", {0.55f, 0.58f, 0.66f}, "siding", {2, 1.5f}, 0.3f, 24.0f),
		{gx0 + 1.2f, Ground, gz1}, {gx1 - 1.2f, Ground + 3.4f, gz1 + 0.08f});
	const float gHalf = (gx1 - gx0) * 0.5f + 0.5f, gRise = 2.0f, gx = (gx0 + gx1) * 0.5f;
	const float gSlope = std::atan2(gRise, gHalf), gSlab = std::sqrt(gHalf * gHalf + gRise * gRise);
	Material& garageShingles = Tinted(a, "garage-shingles", {0.80f, 0.40f, 0.28f}, "shingles", {3, 4}, 0.12f, 12.0f);
	for (float side : {-1.0f, 1.0f})
		ext->AddShape("GarageRoof", &cube, &garageShingles, {gx + side * gHalf * 0.5f, gTop + gRise * 0.5f + 0.1f, (gz0 + gz1) * 0.5f},
			{gSlab, 0.22f, gz1 - gz0 + 1.0f}, {0, 0, -side * glm::degrees(gSlope)});
	Gable(*ext, "GarageGable", cube, Tinted(a, "siding-garage-gable", garageYellow, "siding", {3, 1}, 0.08f, 12.0f),
		{gx, gTop, gz1 - 0.15f}, gHalf - 0.5f, gRise - 0.1f, 0.26f, true);

	// Garden: lawn, path, pavement, driveway, picket fence with a gate, trees, shrubs, mailbox
	Material& lawn = Tinted(a, "lawn", {0.38f, 0.62f, 0.26f}, "grass", {80, 80}, 0.02f, 4.0f);
	ext->AddShape("Lawn", &plane, &lawn, {0, Ground, 0}, {320, 1, 320});
	Material& paving = a.Mat("paving", {0.80f, 0.74f, 0.62f}, 0.05f);
	ext->AddShape("Path", &plane, &paving, {12.0f, Ground + 0.02f, 18.0f}, {1.8f, 1, 9.0f});
	ext->AddShape("Pavement", &plane, &paving, {0.0f, Ground + 0.02f, 23.4f}, {120.0f, 1, 2.6f});
	ext->AddShape("Street", &plane, &a.Mat("street", {0.30f, 0.30f, 0.32f}, 0.05f), {0.0f, Ground + 0.015f, 30.0f}, {200.0f, 1, 10.0f});
	ext->AddShape("Driveway", &plane, &paving, {(gx0 + gx1) * 0.5f, Ground + 0.02f, 15.0f}, {gx1 - gx0 - 1.0f, 1, 15.0f});

	Material& picket = a.Mat("picket-white", {0.97f, 0.97f, 0.95f}, 0.2f, 24.0f);
	const float fenceZ = 21.5f, fenceLeft = gx1 + 0.5f, fenceRight = 26.0f;
	CutoutPanel(*ext, a, "fence-left", {fenceLeft, Ground, fenceZ - 0.04f}, {11.0f, Ground + 1.4f, fenceZ + 0.04f}, 0.7f);
	CutoutPanel(*ext, a, "fence-right", {13.0f, Ground, fenceZ - 0.04f}, {fenceRight, Ground + 1.4f, fenceZ + 0.04f}, 0.7f);
	for (float x : {11.0f, 13.0f})
		Box(*ext, "GatePost", cube, picket, {x - 0.15f, Ground, fenceZ - 0.15f}, {x + 0.15f, Ground + 1.75f, fenceZ + 0.15f});
	Box(*ext, "MailboxPost", cube, a.Mat("tree-bark", {0.36f, 0.25f, 0.16f}, 0.05f), {14.4f, Ground, fenceZ - 0.6f}, {14.55f, Ground + 1.3f, fenceZ - 0.45f});
	ext->AddShape("Mailbox", &cylinder, &trim, {14.48f, Ground + 1.45f, fenceZ - 0.52f}, {0.4f, 0.7f, 0.4f}, {90, 0, 0});

	Material& shrub = a.Mat("shrub", {0.22f, 0.45f, 0.20f}, 0.05f);
	for (float x : {-8.5f, -5.0f, -1.0f, 5.8f, 16.6f})
		ext->AddShape("Shrub", &sphere, &shrub, {x, Ground + 0.35f, HouseFront + 0.9f}, {1.6f, 1.0f, 1.2f});
	Tree(*ext, a, {-26.0f, Ground, 12.0f}, 9.0f);
	Tree(*ext, a, {24.0f, Ground, 14.0f}, 8.0f);
	Tree(*ext, a, {-14.0f, Ground, -15.0f}, 10.0f);
	Tree(*ext, a, {8.0f, Ground, -17.0f}, 11.0f);
	Tree(*ext, a, {30.0f, Ground, -8.0f}, 9.0f);

	// The garden's sun: a large emissive sphere with a translucent halo, positioned every frame by
	// Environment::OutdoorSunPosition() so it sets while Penny walks to the house.
	Material& sunDisc = a.Mat("outdoor-sun", glm::vec3(0.0f), 0.0f);
	sunDisc.unlit = true; sunDisc.emissive = {1.0f, 0.88f, 0.55f};
	Material& halo = a.Mat("outdoor-sun-halo", glm::vec3(0.0f), 0.0f);
	halo.unlit = true; halo.emissive = {1.0f, 0.95f, 0.78f}; halo.opacity = 0.4f;
	rig.outdoorSun = ext->AddChild("OutdoorSun");
	rig.outdoorSun->AddShape("SunDisc", &sphere, &sunDisc, {0, 0, 0}, glm::vec3(7.0f));
	rig.outdoorSun->AddShape("SunHalo", &sphere, &halo, {0, 0, 0}, glm::vec3(13.0f));
	return rig;
}
