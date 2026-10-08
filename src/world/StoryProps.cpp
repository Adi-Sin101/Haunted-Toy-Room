#include "StoryProps.h"

#include <string>

#include "Room.h"
#include "render/Assets.h"
#include "scene/Material.h"
#include "scene/SceneNode.h"

namespace {

using namespace RoomSize;

// A box given by two opposite corners instead of centre and size.
SceneNode* Box(SceneNode& parent, const char* name, const Mesh& cube, const Material& mat, const glm::vec3& lo, const glm::vec3& hi)
{
	return parent.AddShape(name, &cube, &mat, (lo + hi) * 0.5f, hi - lo);
}

Material& Textured(Assets& a, const std::string& name, const glm::vec3& color, int slot, const glm::vec2& repeat,
	float ks = 0.15f, float shininess = 20.0f)
{
	Material& m = a.Mat(name, color, ks, shininess);
	m.texture = a.SlotTexture(slot);
	m.rtTextureSlot = slot;
	m.uvScale = repeat;
	return m;
}

// ---------------------------------------------------------------------------------------------
// Toy chest: a hollow box (floor, four walls) with a painted front panel, iron corner brackets and
// bands, a brass hasp, and a lid made of a deep frame plus a half-cylinder dome. The dome's lower half
// lies inside the lid frame, so the lid reads as a curved lid whether it is closed or open.
// ---------------------------------------------------------------------------------------------
void BuildChest(SceneNode& root, Assets& a, StoryRig& rig)
{
	const Mesh& cube = a.Cube();
	const Mesh& cylinder = a.Cylinder();
	const Mesh& sphere = a.Sphere();
	Material& wood = Textured(a, "chest-wood", {0.66f, 0.27f, 0.16f}, Assets::FloorSlot, {1.2f, 0.5f}, 0.2f, 24.0f);
	wood.plainColor = {0.52f, 0.20f, 0.12f};
	Material& lining = Textured(a, "chest-lining", {0.30f, 0.14f, 0.08f}, Assets::FloorSlot, {1.0f, 0.5f}, 0.05f, 8.0f);
	Material& paint = Textured(a, "chest-painted-front", glm::vec3(1.0f), Assets::ChestSlot, {1, 1}, 0.18f, 18.0f);
	paint.plainColor = {0.55f, 0.16f, 0.10f};
	Material& iron = a.Mat("chest-iron", {0.27f, 0.22f, 0.16f}, 0.55f, 48.0f);
	iron.reflectivity = 0.05f;
	Material& brass = a.Mat("brass", {0.85f, 0.65f, 0.25f}, 0.8f, 64.0f);
	Material& steel = a.Mat("chest-button-ring", {0.62f, 0.62f, 0.64f}, 0.9f, 96.0f);
	Material& button = a.Mat("chest-red-button", {0.90f, 0.10f, 0.06f}, 0.8f, 96.0f);
	button.emissive = {0.55f, 0.04f, 0.0f};
	rig.buttonGlow = &button;
	Material& ink = a.Mat("keyhole", {0.03f, 0.03f, 0.03f}, 0.0f);

	constexpr float L = 3.4f, D = 1.9f, H = 1.05f, frame = 0.42f, dome = 0.42f;
	rig.chestCentre = {0.5f, 0.0f, 0.9f};
	SceneNode* chest = root.AddChild("ToyChest");
	chest->local.position = rig.chestCentre;
	rig.chest = chest;
	auto solid = [](SceneNode* n) { n->solid = true; return n; };

	// Hollow body: the toys hide inside it until the lid opens.
	chest->AddShape("ChestFloor", &cube, &lining, {0, 0.06f, 0}, {L - 0.2f, 0.12f, D - 0.2f});
	solid(chest->AddShape("ChestFront", &cube, &wood, {0, H * 0.5f, D * 0.5f - 0.06f}, {L, H, 0.12f}));
	solid(chest->AddShape("ChestBack", &cube, &wood, {0, H * 0.5f, -D * 0.5f + 0.06f}, {L, H, 0.12f}));
	for (float side : {-1.0f, 1.0f})
		solid(chest->AddShape("ChestEnd", &cube, &wood, {side * (L * 0.5f - 0.06f), H * 0.5f, 0}, {0.12f, H, D - 0.24f}));
	chest->AddShape("ChestInnerLining", &cube, &lining, {0, H * 0.5f, 0}, {L - 0.26f, H - 0.14f, D - 0.26f})->visible = false;
	chest->AddShape("PaintedFront", &cube, &paint, {0, H * 0.5f + 0.02f, D * 0.5f + 0.006f}, {L - 0.42f, H - 0.22f, 0.02f});
	chest->AddShape("BaseBand", &cube, &iron, {0, 0.06f, 0}, {L + 0.05f, 0.12f, D + 0.05f});
	chest->AddShape("RimBand", &cube, &iron, {0, H - 0.04f, 0}, {L + 0.04f, 0.08f, D + 0.04f});
	for (float x : {-1.0f, 1.0f}) for (float z : {-1.0f, 1.0f}) {
		chest->AddShape("CornerBracket", &cube, &iron, {x * (L * 0.5f - 0.05f), H * 0.5f, z * (D * 0.5f - 0.05f)}, {0.17f, H + 0.02f, 0.17f});
		for (float y : {0.25f, 0.75f})
			chest->AddShape("Rivet", &sphere, &brass, {x * (L * 0.5f - 0.05f), y, z * (D * 0.5f + 0.04f)}, glm::vec3(0.06f));
	}
	SceneNode* lockPlate = chest->AddShape("LockPlate", &cube, &brass, {0, H - 0.28f, D * 0.5f + 0.03f}, {0.32f, 0.30f, 0.04f});
	(void)lockPlate;
	chest->AddShape("Keyhole", &cube, &ink, {0, H - 0.30f, D * 0.5f + 0.052f}, {0.05f, 0.12f, 0.01f});
	for (float side : {-1.0f, 1.0f})
		chest->AddShape("SideHandle", &cube, &brass, {side * (L * 0.5f + 0.04f), H * 0.62f, 0}, {0.05f, 0.12f, 0.55f});

	// Wind-up key on the right end: it spins while the lid springs open.
	rig.chestKey = chest->AddChild("WindUpKey");
	rig.chestKey->local.position = {L * 0.5f + 0.10f, H * 0.45f, 0};
	rig.chestKey->AddShape("KeyStem", &cylinder, &brass, {0.08f, 0, 0}, {0.07f, 0.2f, 0.07f}, {0, 0, 90});
	rig.chestKey->AddShape("KeyWings", &sphere, &brass, {0.2f, 0, 0}, {0.05f, 0.42f, 0.2f});

	// Lid: hinge joint on the back top edge; everything below is in lid space (front = +Z).
	rig.chestLid = chest->AddChild("ChestLid");
	rig.chestLid->local.position = {0, H, -D * 0.5f};
	SceneNode* lid = rig.chestLid;
	lid->AddShape("LidFrame", &cube, &wood, {0, frame * 0.5f, D * 0.5f}, {L, frame, D});
	lid->AddShape("LidDome", &cylinder, &wood, {0, frame, D * 0.5f}, {dome * 2.0f, L - 0.02f, D - 0.02f}, {0, 0, 90});
	lid->AddShape("LidBand", &cube, &iron, {0, 0.035f, D * 0.5f}, {L + 0.04f, 0.07f, D + 0.04f});
	for (float x : {-L * 0.5f + 0.09f, -L * 0.22f, L * 0.22f, L * 0.5f - 0.09f}) {
		lid->AddShape("DomeStrap", &cylinder, &iron, {x, frame, D * 0.5f}, {dome * 2.0f + 0.05f, 0.16f, D + 0.05f}, {0, 0, 90});
		lid->AddShape("FrameStrap", &cube, &iron, {x, frame * 0.5f, D * 0.5f}, {0.16f, frame, D + 0.05f});
	}
	lid->AddShape("Hasp", &cube, &brass, {0, 0.10f, D + 0.035f}, {0.26f, 0.34f, 0.04f});
	// The big red wind-up button on top of the dome, in a riveted steel ring.
	lid->AddShape("ButtonRing", &cylinder, &steel, {0, frame + dome - 0.02f, D * 0.5f}, {0.56f, 0.08f, 0.56f});
	rig.chestButton = lid->AddChild("ChestButton");
	rig.chestButton->local.position = {0, frame + dome + 0.02f, D * 0.5f};
	rig.chestButton->AddShape("ButtonBody", &cylinder, &button, {0, 0.03f, 0}, {0.38f, 0.10f, 0.38f});
	rig.chestButton->AddShape("ButtonCap", &sphere, &button, {0, 0.08f, 0}, {0.38f, 0.16f, 0.38f});
}

// A wooden door leaf of `width` x `height`, hinged at the joint origin and extending along `dir`
// (+1 or -1) of local Z, with two raised panels and a brass knob.
void DoorLeaf(SceneNode& hinge, Assets& a, const Material& wood, const Material& panel, float width, float height, float dir)
{
	const Mesh& cube = a.Cube();
	hinge.AddShape("DoorLeaf", &cube, &wood, {0, height * 0.5f, dir * width * 0.5f}, {0.09f, height - 0.04f, width - 0.04f})->solid = true;
	for (float y : {height * 0.28f, height * 0.70f})
		hinge.AddShape("DoorPanel", &cube, &panel, {0, y, dir * width * 0.5f}, {0.12f, height * 0.32f, width - 0.55f});
	hinge.AddShape("DoorKnob", &a.Sphere(), &a.Mat("brass", {0.85f, 0.65f, 0.25f}, 0.8f, 64.0f),
		{0, height * 0.45f, dir * (width - 0.25f)}, {0.24f, 0.13f, 0.13f});
}

// ---------------------------------------------------------------------------------------------
// The wardrobe Buzz is trapped in (front faces +X): a hollow carcass on a plinth, an arched crown
// with ball finials, two doors with arched raised panels, gold star decals and star knobs.
// The knobs are high on the doors, beyond Penny's and Jessie's reach.
// ---------------------------------------------------------------------------------------------
void BuildWardrobe(SceneNode& room, Assets& a, StoryRig& rig, Material& wood, Material& panel)
{
	const Mesh& cube = a.Cube();
	const Mesh& cylinder = a.Cylinder();
	const Mesh& sphere = a.Sphere();
	Material& inside = Textured(a, "wardrobe-inside", {0.36f, 0.22f, 0.13f}, Assets::FloorSlot, {0.6f, 1.0f}, 0.05f, 8.0f);
	Material& star = Textured(a, "gold-star-decal", glm::vec3(1.0f), Assets::StarDecalSlot, {1, 1}, 0.6f, 64.0f);
	star.cutout = true;
	Material& brass = a.Mat("brass", {0.85f, 0.65f, 0.25f}, 0.8f, 64.0f);
	Material& glow = a.Mat("wardrobe-buzz-glow", glm::vec3(0.0f), 0.0f);
	glow.unlit = true; glow.emissive = {0.25f, 0.95f, 0.45f}; glow.opacity = 0.85f;
	rig.wardrobeGlow = &glow;

	constexpr float depth = 1.25f, width = 3.0f, body = 3.45f;
	const float floorY = Ground + 0.02f;
	const glm::vec3 c(BuzzLeft + depth * 0.5f + 0.05f, floorY, -5.3f);
	SceneNode* w = room.AddChild("Wardrobe");
	w->local.position = c;
	rig.wardrobeInside = c + glm::vec3(0.05f, 0.38f, 0); // on the inner floor board
	rig.wardrobeFront = c + glm::vec3(depth * 0.5f + 1.05f, 0, 0); // beside the doors, heading +Z
	auto solid = [](SceneNode* n) { n->solid = true; return n; };

	// Carcass: plinth, floor, back, sides and top (open at the front behind the doors).
	solid(Box(*w, "Plinth", cube, wood, {-depth * 0.5f - 0.04f, 0, -width * 0.5f - 0.04f}, {depth * 0.5f + 0.06f, 0.30f, width * 0.5f + 0.04f}));
	Box(*w, "InnerFloor", cube, inside, {-depth * 0.5f, 0.30f, -width * 0.5f + 0.08f}, {depth * 0.5f - 0.05f, 0.36f, width * 0.5f - 0.08f});
	solid(Box(*w, "BackPanel", cube, inside, {-depth * 0.5f, 0.30f, -width * 0.5f}, {-depth * 0.5f + 0.08f, body, width * 0.5f}));
	for (float side : {-1.0f, 1.0f})
		solid(Box(*w, "SidePanel", cube, wood, {-depth * 0.5f, 0.30f, side > 0 ? width * 0.5f - 0.09f : -width * 0.5f},
			{depth * 0.5f, body, side > 0 ? width * 0.5f : -width * 0.5f + 0.09f}));
	Box(*w, "Top", cube, wood, {-depth * 0.5f - 0.05f, body - 0.12f, -width * 0.5f - 0.06f}, {depth * 0.5f + 0.08f, body + 0.06f, width * 0.5f + 0.06f});
	Box(*w, "Cornice", cube, panel, {depth * 0.5f - 0.02f, body - 0.30f, -width * 0.5f - 0.02f}, {depth * 0.5f + 0.06f, body - 0.12f, width * 0.5f + 0.02f});
	w->AddShape("HangingRail", &cylinder, &brass, {-0.1f, body - 0.5f, 0}, {0.06f, width - 0.2f, 0.06f}, {90, 0, 0});
	// Arched crown: a flattened cylinder whose lower half lies inside the top board.
	w->AddShape("CrownArch", &cylinder, &wood, {depth * 0.5f - 0.10f, body, 0}, {1.0f, 0.16f, 1.9f}, {0, 0, 90});
	w->AddShape("CrownArchTrim", &cylinder, &panel, {depth * 0.5f - 0.06f, body, 0}, {0.82f, 0.12f, 1.55f}, {0, 0, 90});
	for (float side : {-1.0f, 1.0f}) {
		w->AddShape("FinialPost", &cube, &wood, {depth * 0.5f - 0.05f, body + 0.16f, side * (width * 0.5f - 0.10f)}, {0.18f, 0.22f, 0.18f});
		w->AddShape("FinialBall", &sphere, &wood, {depth * 0.5f - 0.05f, body + 0.36f, side * (width * 0.5f - 0.10f)}, glm::vec3(0.28f));
	}

	// Doors: hinged at the outer edges, meeting in the middle.
	const float doorH = body - 0.30f - 0.36f, doorW = width * 0.5f - 0.08f, front = depth * 0.5f + 0.03f;
	for (int side = 0; side < 2; ++side) {
		const float dir = side == 0 ? -1.0f : 1.0f;                  // left leaf extends -Z from a +Z hinge
		SceneNode* hinge = w->AddChild(side == 0 ? "WardrobeDoorLeft" : "WardrobeDoorRight");
		hinge->local.position = {front, 0.36f, side == 0 ? width * 0.5f - 0.08f : -width * 0.5f + 0.08f};
		hinge->AddShape("WardrobeDoor", &cube, &wood, {0, doorH * 0.5f, dir * doorW * 0.5f}, {0.07f, doorH, doorW - 0.02f});
		// Raised panel with an arched top: box + flattened cylinder, slightly thinner so it never z-fights.
		const float pz = dir * doorW * 0.5f, pw = doorW - 0.36f;
		hinge->AddShape("DoorPanel", &cube, &panel, {0.04f, doorH * 0.43f, pz}, {0.06f, doorH * 0.62f, pw});
		hinge->AddShape("DoorPanelArch", &cylinder, &panel, {0.04f, doorH * 0.74f, pz}, {0.5f, 0.055f, pw}, {0, 0, 90});
		hinge->AddShape("StarDecal", &cube, &star, {0.078f, doorH * 0.70f, pz}, {0.012f, 0.40f, 0.40f});
		hinge->AddShape("StarKnob", &sphere, &brass, {0.09f, doorH - 0.5f, dir * (doorW - 0.13f)}, {0.12f, 0.17f, 0.17f});
		hinge->AddShape("KnobStar", &cube, &star, {0.155f, doorH - 0.5f, dir * (doorW - 0.13f)}, {0.01f, 0.22f, 0.22f});
		for (float y : {0.25f, doorH - 0.25f})
			hinge->AddShape("Hinge", &cube, &brass, {0.0f, y, 0.0f}, {0.09f, 0.20f, 0.05f});
		(side == 0 ? rig.wardrobeLeft : rig.wardrobeRight) = hinge;
	}
	// A thin line of green light leaks between the closed doors: something inside is glowing.
	w->AddShape("DoorGapGlow", &cube, &glow, {front - 0.02f, 0.36f + doorH * 0.5f, 0}, {0.03f, doorH - 0.3f, 0.05f});
}

// ---------------------------------------------------------------------------------------------
// Buzz's bedroom: an ordinary furnished room, clearly different from the Toy Room above.
// ---------------------------------------------------------------------------------------------
void BuildBuzzRoom(SceneNode& root, Assets& a, StoryRig& rig)
{
	const Mesh& plane = a.Plane();
	const Mesh& cube = a.Cube();
	const Mesh& cylinder = a.Cylinder();
	const Mesh& sphere = a.Sphere();
	const Mesh& cone = a.Cone();
	SceneNode* room = root.AddChild("BuzzRoom");
	const float floorY = Ground + 0.02f, h = Slab - Ground;
	const float width = CorridorLeft - BuzzLeft, depth = BuzzFront + 9.05f;
	const float cx = (BuzzLeft + CorridorLeft) * 0.5f, cz = (BuzzFront - 9.05f) * 0.5f;
	auto solid = [](SceneNode* n) { n->solid = true; return n; };

	Material& floor = Textured(a, "buzz-room-floor", {0.95f, 0.78f, 0.62f}, Assets::FloorSlot, {3.0f, 2.5f}, 0.3f, 40.0f);
	floor.reflectivity = 0.12f;
	room->AddShape("BuzzRoomFloor", &plane, &floor, {cx, floorY, cz}, {width, 1, depth});
	room->AddShape("BuzzRoomCeiling", &plane, &a.Mat("ceiling", {0.85f, 0.83f, 0.78f}, 0.0f), {cx, Slab, cz}, {width, 1, depth}, {180, 0, 0});
	auto wallpaper = [&](const std::string& name, float w, float hh) -> Material& {
		Material& m = Textured(a, "buzz-wall-" + name, glm::vec3(1.0f), Assets::StarWallSlot, {w / 2.4f, hh / 2.4f}, 0.05f, 8.0f);
		m.plainColor = {0.22f, 0.30f, 0.50f};
		return m;
	};
	// One-sided walls facing into the room (rotation (90, yaw, 0): yaw 90 faces +X, -90 faces -X, 0 faces +Z).
	auto wall = [&](const char* name, const glm::vec3& centre, float yaw, float w, float hh) {
		room->AddShape(name, &plane, &wallpaper(name, w, hh), centre, {w, 1, hh}, {90, yaw, 0});
	};
	wall("BackWall", {BuzzLeft, Ground + h * 0.5f, cz}, 90, depth, h);
	wall("WindowWall", {cx, Ground + h * 0.5f, -9.05f}, 0, width, h);
	wall("FrontWall", {cx, Ground + h * 0.5f, BuzzFront}, 180, width, h);
	// The corridor side, around the doorway (the solid divider itself belongs to the house).
	const float doorMid = (BuzzDoorLow + BuzzDoorHigh) * 0.5f;
	wall("DoorWallBack", {CorridorLeft - 0.09f, Ground + h * 0.5f, (BuzzDoorLow - 9.05f) * 0.5f}, -90, BuzzDoorLow + 9.05f, h);
	wall("DoorWallFront", {CorridorLeft - 0.09f, Ground + h * 0.5f, (BuzzDoorHigh + BuzzFront) * 0.5f}, -90, BuzzFront - BuzzDoorHigh, h);
	wall("DoorWallLintel", {CorridorLeft - 0.09f, (Ground + BuzzDoorHeight + Slab) * 0.5f, doorMid}, -90, BuzzDoorHigh - BuzzDoorLow, Slab - Ground - BuzzDoorHeight);
	Material& trim = a.Mat("buzz-room-trim", {0.93f, 0.90f, 0.82f}, 0.25f, 24.0f);
	room->AddShape("Skirting", &cube, &trim, {BuzzLeft + 0.05f, Ground + 0.15f, cz}, {0.1f, 0.3f, depth});
	room->AddShape("Skirting", &cube, &trim, {cx, Ground + 0.15f, -9.0f}, {width, 0.3f, 0.1f});
	room->AddShape("Skirting", &cube, &trim, {cx, Ground + 0.15f, BuzzFront - 0.05f}, {width, 0.3f, 0.1f});
	room->AddShape("CrownMoulding", &cube, &trim, {cx, Slab - 0.08f, -9.0f}, {width, 0.16f, 0.12f});
	room->AddShape("CrownMoulding", &cube, &trim, {cx, Slab - 0.08f, BuzzFront - 0.06f}, {width, 0.16f, 0.12f});
	room->AddShape("CrownMoulding", &cube, &trim, {BuzzLeft + 0.06f, Slab - 0.08f, cz}, {0.12f, 0.16f, depth});

	Material& wood = Textured(a, "wardrobe-wood", {0.86f, 0.55f, 0.33f}, Assets::FloorSlot, {0.45f, 1.0f}, 0.3f, 36.0f);
	wood.plainColor = {0.55f, 0.32f, 0.17f};
	Material& panel = Textured(a, "wardrobe-panel", {0.74f, 0.45f, 0.26f}, Assets::FloorSlot, {0.3f, 0.8f}, 0.3f, 36.0f);
	BuildWardrobe(*room, a, rig, wood, panel);

	// Window with a night pane and blue plaid curtains on the outer wall.
	const float wx = 7.3f, wy = Ground + 2.45f;
	Material& pane = a.Mat("buzz-window-pane", glm::vec3(1.0f), 0.9f, 96.0f);
	pane.texture = a.Named("window-pane"); pane.rtTextureSlot = a.NamedSlot("window-pane");
	pane.plainColor = {0.12f, 0.18f, 0.30f}; pane.reflectivity = 0.2f;
	room->AddShape("WindowFrame", &cube, &trim, {wx, wy, -9.0f}, {2.3f, 2.1f, 0.08f});
	room->AddShape("WindowPane", &cube, &pane, {wx, wy, -8.95f}, {2.0f, 1.8f, 0.04f});
	room->AddShape("WindowSill", &cube, &trim, {wx, wy - 1.05f, -8.85f}, {2.5f, 0.08f, 0.32f});
	Material& curtain = Textured(a, "buzz-curtain", {0.55f, 0.65f, 0.95f}, Assets::PlaidSlot, {2, 4}, 0.0f, 8.0f);
	for (float side : {-1.0f, 1.0f}) for (int fold = 0; fold < 3; ++fold)
		room->AddShape("CurtainFold", &cylinder, &curtain, {wx + side * (1.25f + fold * 0.2f), wy - 0.2f, -8.82f}, {0.24f, 2.9f, 0.12f});
	room->AddShape("CurtainRod", &cylinder, &a.Mat("brass", {0.85f, 0.65f, 0.25f}, 0.8f, 64.0f), {wx, wy + 1.3f, -8.8f}, {0.05f, 3.3f, 0.05f}, {0, 0, 90});

	// Bed against the front wall: frame with ball-topped posts, mattress, patchwork star quilt, pillow.
	Material& bedWood = Textured(a, "buzz-bed-wood", {0.80f, 0.50f, 0.30f}, Assets::FloorSlot, {0.6f, 0.6f}, 0.3f, 32.0f);
	Material& quilt = Textured(a, "buzz-star-quilt", glm::vec3(1.0f), Assets::QuiltSlot, {1.5f, 2.0f}, 0.02f, 8.0f);
	quilt.plainColor = {0.20f, 0.28f, 0.52f};
	Material& linen = a.Mat("buzz-linen", {0.92f, 0.90f, 0.84f}, 0.05f, 8.0f);
	SceneNode* bed = room->AddChild("BuzzRoomBed");
	bed->local.position = {7.6f, floorY, -3.2f};
	solid(bed->AddShape("BedFrame", &cube, &bedWood, {0, 0.38f, 0}, {2.5f, 0.55f, 3.2f}));
	bed->AddShape("Mattress", &cube, &linen, {0, 0.78f, 0}, {2.4f, 0.28f, 3.1f});
	bed->AddShape("Quilt", &cube, &quilt, {0, 0.90f, -0.25f}, {2.56f, 0.12f, 2.65f});
	bed->AddShape("QuiltDrop", &cube, &quilt, {0, 0.62f, -1.56f}, {2.56f, 0.6f, 0.06f});
	bed->AddShape("Pillow", &sphere, &linen, {0, 1.02f, 1.05f}, {1.5f, 0.3f, 0.7f});
	solid(bed->AddShape("Headboard", &cube, &bedWood, {0, 1.0f, 1.55f}, {2.5f, 1.6f, 0.14f}));
	for (float x : {-1.2f, 1.2f}) for (float z : {-1.55f, 1.55f}) {
		const float post = z > 0 ? 1.9f : 1.15f;
		bed->AddShape("BedPost", &cylinder, &bedWood, {x, post * 0.5f, z}, {0.18f, post, 0.18f});
		bed->AddShape("PostBall", &sphere, &bedWood, {x, post + 0.08f, z}, glm::vec3(0.26f));
	}

	// Bookcase beside the wardrobe: shelves of books, a globe, a toy rocket, star boxes and a teddy bear.
	Material& books = Textured(a, "buzz-book-spines", glm::vec3(1.0f), Assets::BookSlot, {1, 1}, 0.08f, 16.0f);
	books.plainColor = {0.42f, 0.22f, 0.16f};
	SceneNode* shelf = room->AddChild("BuzzRoomBookcase");
	shelf->local.position = {BuzzLeft + 0.5f, floorY, -2.55f};
	for (float side : {-1.0f, 1.0f}) solid(shelf->AddShape("Upright", &cube, &bedWood, {0, 1.5f, side * 0.78f}, {0.9f, 3.0f, 0.1f}));
	solid(shelf->AddShape("Back", &cube, &bedWood, {-0.42f, 1.5f, 0}, {0.06f, 3.0f, 1.6f}));
	for (int tier = 0; tier < 4; ++tier) {
		const float level = 0.12f + tier * 0.95f;
		solid(shelf->AddShape("Shelf", &cube, &bedWood, {0, level, 0}, {0.9f, 0.08f, 1.5f}));
		if (tier < 2) shelf->AddShape("Books", &cube, &books, {-0.05f, level + 0.36f, -0.2f}, {1.0f, 0.64f, 0.55f}, {0, 90, 0});
	}
	Material& block = Textured(a, "buzz-star-box", {0.30f, 0.45f, 0.85f}, Assets::BlockSlot, {1, 1}, 0.2f, 24.0f);
	Material& blockRed = Textured(a, "buzz-star-box-red", {0.85f, 0.30f, 0.25f}, Assets::BlockSlot, {1, 1}, 0.2f, 24.0f);
	shelf->AddShape("StarBox", &cube, &block, {0.0f, 0.12f + 0.04f + 0.25f, 0.45f}, glm::vec3(0.5f));
	shelf->AddShape("StarBox", &cube, &blockRed, {0.0f, 1.07f + 0.04f + 0.22f, 0.48f}, glm::vec3(0.44f));
	Material& globe = a.Mat("buzz-globe", {0.25f, 0.50f, 0.85f}, 0.6f, 64.0f);
	shelf->AddShape("GlobeStand", &cylinder, &bedWood, {0, 2.02f + 0.06f, 0.1f}, {0.3f, 0.12f, 0.3f});
	shelf->AddShape("Globe", &sphere, &globe, {0, 2.02f + 0.42f, 0.1f}, glm::vec3(0.62f));
	Material& white = a.Mat("buzz-toy-white", {0.92f, 0.92f, 0.90f}, 0.5f, 48.0f);
	Material& red = a.Mat("buzz-toy-red", {0.85f, 0.15f, 0.12f}, 0.5f, 48.0f);
	shelf->AddShape("ToyRocket", &cylinder, &white, {0, 2.97f + 0.36f, -0.4f}, {0.2f, 0.6f, 0.2f});
	shelf->AddShape("ToyRocketNose", &cone, &red, {0, 2.97f + 0.78f, -0.4f}, {0.2f, 0.26f, 0.2f});
	shelf->AddShape("ToyRocketFins", &cube, &red, {0, 2.97f + 0.12f, -0.4f}, {0.04f, 0.2f, 0.42f});
	Material& bear = a.Mat("teddy-fur", {0.55f, 0.36f, 0.20f}, 0.05f, 8.0f);
	bear.texture = a.SlotTexture(Assets::FabricSlot); bear.rtTextureSlot = Assets::FabricSlot; bear.uvScale = {3, 3};
	SceneNode* teddy = shelf->AddChild("TeddyBear");
	teddy->local.position = {0.05f, 1.07f + 0.04f, -0.25f};
	teddy->local.rotation.y = 90.0f;
	teddy->AddShape("Body", &sphere, &bear, {0, 0.30f, 0}, {0.48f, 0.55f, 0.40f});
	teddy->AddShape("Head", &sphere, &bear, {0, 0.72f, 0.02f}, glm::vec3(0.38f));
	for (float side : {-1.0f, 1.0f}) {
		teddy->AddShape("Ear", &sphere, &bear, {side * 0.15f, 0.88f, 0}, glm::vec3(0.14f));
		teddy->AddShape("Paw", &sphere, &bear, {side * 0.18f, 0.10f, 0.14f}, glm::vec3(0.18f));
	}
	teddy->AddShape("Bow", &cube, &red, {0, 0.53f, 0.17f}, {0.24f, 0.08f, 0.04f});

	// Nightstand with a lamp: the room's warm light.
	SceneNode* stand = room->AddChild("Nightstand");
	stand->local.position = {3.55f, floorY, -8.35f};
	solid(stand->AddShape("Cabinet", &cube, &bedWood, {0, 0.6f, 0}, {1.1f, 1.2f, 1.0f}));
	for (float y : {0.35f, 0.85f}) {
		stand->AddShape("Drawer", &cube, &panel, {0, y, 0.51f}, {0.9f, 0.38f, 0.03f});
		stand->AddShape("DrawerKnob", &sphere, &a.Mat("brass", {0.85f, 0.65f, 0.25f}, 0.8f, 64.0f), {0, y, 0.55f}, glm::vec3(0.08f));
	}
	Material& shade = a.Mat("buzz-lamp-shade", {0.95f, 0.82f, 0.55f}, 0.1f, 8.0f);
	shade.emissive = {0.35f, 0.25f, 0.10f};
	Material& bulb = a.Mat("buzz-lamp-bulb", glm::vec3(0.0f), 0.0f);
	bulb.unlit = true; bulb.emissive = {1.0f, 0.85f, 0.55f};
	stand->AddShape("LampBase", &cylinder, &a.Mat("brass", {0.85f, 0.65f, 0.25f}, 0.8f, 64.0f), {0, 1.25f, 0}, {0.3f, 0.1f, 0.3f});
	stand->AddShape("LampStem", &cylinder, &a.Mat("brass", {0.85f, 0.65f, 0.25f}, 0.8f, 64.0f), {0, 1.55f, 0}, {0.06f, 0.6f, 0.06f});
	stand->AddShape("LampBulb", &sphere, &bulb, {0, 1.82f, 0}, glm::vec3(0.16f));
	stand->AddShape("LampShade", &cone, &shade, {0, 2.0f, 0}, {0.62f, 0.45f, 0.62f});
	rig.nightLamp = stand->local.position + glm::vec3(0, 1.85f, 0.15f);

	// "To infinity and beyond" poster, star rug, football and storage boxes.
	Material& poster = Textured(a, "buzz-room-poster", glm::vec3(1.0f), Assets::PosterSlot, {1, 1}, 0.2f, 32.0f);
	room->AddShape("BuzzPoster", &plane, &poster, {5.6f, Ground + 2.35f, -9.03f}, {1.4f, 1, 2.1f}, {90, 0, 0});
	Material& rug = Textured(a, "buzz-star-rug", {0.80f, 0.85f, 1.05f}, Assets::StarWallSlot, {2.0f, 1.5f}, 0.0f, 4.0f);
	rug.plainColor = {0.25f, 0.33f, 0.55f};
	room->AddShape("StarRug", &plane, &rug, {6.2f, floorY + 0.01f, -5.6f}, {4.2f, 1, 3.2f});
	Material& football = Textured(a, "football", glm::vec3(1.0f), Assets::SoccerSlot, {1, 1}, 0.4f, 48.0f);
	room->AddShape("Football", &sphere, &football, {3.3f, floorY + 0.36f, -7.25f}, glm::vec3(0.72f));
	solid(room->AddShape("StorageBox", &cube, &block, {9.4f, floorY + 0.35f, -8.4f}, {0.9f, 0.7f, 0.9f}));
	room->AddShape("StorageBox", &cube, &blockRed, {9.4f, floorY + 0.98f, -8.45f}, {0.6f, 0.56f, 0.6f}, {0, 18, 0});
}

} // namespace

StoryRig BuildStoryProps(SceneNode& root, Assets& assets)
{
	StoryRig rig;
	BuildChest(root, assets, rig);
	BuildBuzzRoom(root, assets, rig);

	// Ordinary door into Buzz's room: hinged on the doorway's front jamb, it opens into the room.
	Material& doorWood = assets.Mat("door-wood", {0.80f, 0.64f, 0.48f}, 0.25f, 24.0f);
	Material& doorPanel = assets.Mat("door-panel", {0.70f, 0.54f, 0.40f}, 0.2f, 24.0f);
	rig.buzzDoor = root.AddChild("BuzzRoomDoor");
	rig.buzzDoor->local.position = {CorridorLeft, Ground + 0.02f, BuzzDoorHigh};
	DoorLeaf(*rig.buzzDoor, assets, doorWood, doorPanel, BuzzDoorHigh - BuzzDoorLow, BuzzDoorHeight - 0.04f, -1.0f);
	return rig;
}
