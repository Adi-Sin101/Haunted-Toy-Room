#include "Room.h"

#include "render/Assets.h"
#include "scene/SceneNode.h"

namespace {

Material& Textured(Assets& a, const std::string& name, const glm::vec3& color, int slot, const glm::vec2& uvScale,
	float ks = 0.1f, float shininess = 16.0f)
{
	Material& m = a.Mat(name, color, ks, shininess);
	m.texture = a.SlotTexture(slot);
	m.rtTextureSlot = slot;
	m.uvScale = uvScale;
	// Keep room surfaces readable in the texture-free preview, without changing textured colours.
	if (slot == Assets::FloorSlot) m.plainColor=color*glm::vec3(0.55f,0.35f,0.20f);
	if (slot == Assets::WallSlot) m.plainColor={0.23f,0.29f,0.43f};
	if (slot == Assets::RugSlot) m.plainColor={0.62f,0.25f,0.20f};
	if (slot == Assets::PosterSlot) m.plainColor={0.18f,0.23f,0.37f};
	if (slot == Assets::BallSlot) m.plainColor={0.80f,0.20f,0.25f};
	if (slot == Assets::MoonSlot) m.plainColor={0.72f,0.73f,0.75f};
	return m;
}

// A wall is a unit plane rotated to stand upright: rotation (90, yaw, 0) turns the plane's +Y normal
// into the horizontal direction given by yaw, its local X into the wall's width and its local Z into
// the wall's height (so the texture stays upright). size = (width, height).
SceneNode* AddWall(SceneNode& parent, const std::string& name, const Mesh& plane, const Material& mat,
	const glm::vec3& center, float yawDeg, const glm::vec2& size)
{
	return parent.AddShape(name, &plane, &mat, center, glm::vec3(size.x, 1.0f, size.y), glm::vec3(90.0f, yawDeg, 0.0f));
}

} // namespace

RoomRig BuildRoom(SceneNode& root, Assets& a)
{
	using namespace RoomSize;
	RoomRig rig;
	const Mesh& plane = a.Plane();
	const Mesh& cube = a.Cube();
	const Mesh& sphere = a.Sphere();
	const Mesh& cylinder = a.Cylinder();
	const Mesh& cone = a.Cone();
	auto solid = [](SceneNode* n) { n->solid = true; return n; };

	SceneNode* room = root.AddChild("Room");

	// ---- Floor and ceiling ------------------------------------------------------------------
	Material& floorMat = Textured(a, "floor", glm::vec3(1.0f), Assets::FloorSlot, { 8.0f, 4.0f }, 0.35f, 48.0f);
	floorMat.reflectivity = 0.18f; // polished wood: visible reflections in ray-traced mode
	room->AddShape("Floor", &plane, &floorMat, { 0, 0, 0 }, { 2 * HalfWidth, 1, 2 * HalfDepth });

	Material& ceilingMat = a.Mat("ceiling", { 0.85f, 0.83f, 0.78f }, 0.0f);
	room->AddShape("Ceiling", &plane, &ceilingMat, { 0, Height, 0 }, { 2 * HalfWidth, 1, 2 * HalfDepth }, { 180, 0, 0 });

	// ---- Walls (wallpaper tiles once per ~2 units) -----------------------------------------
	auto wallMat = [&](const std::string& name, const glm::vec2& size) -> Material& {
		return Textured(a, name, glm::vec3(1.0f), Assets::WallSlot, size / 2.0f, 0.05f, 8.0f);
	};
	const float wy = Height / 2.0f;
	AddWall(*room, "LeftWall", plane, wallMat("wall-side", { 2 * HalfDepth, Height }), { -HalfWidth, wy, 0 }, 90.0f, { 2 * HalfDepth, Height });
	// A real opening leads to the connected hallway; the camera remains in the room.
	for (const auto& segment : std::vector<glm::vec2>{{-HalfDepth, DoorLow}, {DoorHigh, HalfDepth}}) {
		const glm::vec2 size(segment.y - segment.x, Height);
		solid(AddWall(*room, "RightWallPiece", plane, wallMat("wall-side", size),
			{HalfWidth, wy, (segment.x + segment.y) * 0.5f}, -90, size));
	}
	solid(AddWall(*room, "DoorLintel", plane, wallMat("door-lintel", {DoorHigh-DoorLow, Height-DoorHeight}),
		{HalfWidth, (DoorHeight+Height)*0.5f, (DoorLow+DoorHigh)*0.5f}, -90, {DoorHigh-DoorLow,Height-DoorHeight}));
	Material& hallMat = Textured(a, "hall-floor", {0.72f,0.68f,0.59f}, Assets::FloorSlot, {3,3});
	room->AddShape("HallFloor", &plane, &hallMat, {(HalfWidth+HallEnd)*0.5f,0,4}, {HallEnd-HalfWidth,1,DoorHigh-DoorLow});
	Material& hallWall = a.Mat("hall-wall", {0.34f,0.37f,0.42f}, 0.04f);
	// Hall side walls; the z = DoorLow wall has an opening where the stairs arrive from the ground floor.
	solid(room->AddShape("HallSide", &cube, &hallWall, {(HalfWidth+HallEnd)*0.5f,DoorHeight*0.5f,DoorHigh}, {HallEnd-HalfWidth,DoorHeight,0.16f}));
	solid(room->AddShape("HallSide", &cube, &hallWall, {(HalfWidth+StairLeft)*0.5f,DoorHeight*0.5f,DoorLow}, {StairLeft-HalfWidth,DoorHeight,0.16f}));
	solid(room->AddShape("HallSide", &cube, &hallWall, {(StairRight+HallEnd)*0.5f,DoorHeight*0.5f,DoorLow}, {HallEnd-StairRight,DoorHeight,0.16f}));
	solid(room->AddShape("HallEnd", &cube, &hallWall, {HallEnd,DoorHeight*0.5f,4}, {0.16f,DoorHeight,6}));
	solid(room->AddShape("HallCeiling", &cube, &hallWall, {13.5f,DoorHeight,4}, {7,0.15f,6}));
	Material& doorway = a.Mat("door-frame", {0.82f,0.77f,0.65f},0.2f);
	for (float side : {DoorLow,DoorHigh}) solid(room->AddShape("DoorJamb",&cube,&doorway,
		{HalfWidth,DoorHeight*0.5f,side},{0.22f,DoorHeight,0.18f}));
	room->AddShape("DoorHeader",&cube,&doorway,{HalfWidth,DoorHeight,4},{0.22f,0.18f,6.2f});
	AddWall(*room, "FrontWall", plane, wallMat("wall-front", { 2 * HalfWidth, Height }), { 0, wy, HalfDepth }, 180.0f, { 2 * HalfWidth, Height });

	// Back wall = 4 pieces around the window opening x in [0.5, 4.5], y in [2.5, 5.5]
	const float winL = 0.5f, winR = 4.5f, winB = 2.5f, winT = 5.5f;
	const float z = -HalfDepth;
	{
		const glm::vec2 left(winL + HalfWidth, Height), right(HalfWidth - winR, Height);
		const glm::vec2 bottom(winR - winL, winB), top(winR - winL, Height - winT);
		AddWall(*room, "BackWallLeft", plane, wallMat("wall-back-left", left), { (-HalfWidth + winL) / 2, wy, z }, 0.0f, left);
		AddWall(*room, "BackWallRight", plane, wallMat("wall-back-right", right), { (winR + HalfWidth) / 2, wy, z }, 0.0f, right);
		AddWall(*room, "BackWallBottom", plane, wallMat("wall-back-bottom", bottom), { (winL + winR) / 2, winB / 2, z }, 0.0f, bottom);
		AddWall(*room, "BackWallTop", plane, wallMat("wall-back-top", top), { (winL + winR) / 2, (winT + Height) / 2, z }, 0.0f, top);
	}

	// Window frame (cubes) and cross mullions
	Material& frameMat = a.Mat("window-frame", { 0.92f, 0.90f, 0.85f }, 0.3f, 32.0f);
	const float cx = (winL + winR) / 2, cy = (winB + winT) / 2, fw = 0.18f, fd = 0.3f;
	SceneNode* window = room->AddChild("Window");
	window->AddShape("FrameTop", &cube, &frameMat, { cx, winT, z }, { winR - winL + fw, fw, fd });
	window->AddShape("FrameBottom", &cube, &frameMat, { cx, winB, z }, { winR - winL + fw, fw, fd });
	window->AddShape("FrameLeft", &cube, &frameMat, { winL, cy, z }, { fw, winT - winB, fd });
	window->AddShape("FrameRight", &cube, &frameMat, { winR, cy, z }, { fw, winT - winB, fd });
	window->AddShape("MullionV", &cube, &frameMat, { cx, cy, z }, { 0.07f, winT - winB, 0.08f });
	window->AddShape("MullionH", &cube, &frameMat, { cx, cy, z }, { winR - winL, 0.07f, 0.08f });
	window->AddShape("Sill", &cube, &frameMat, { cx, winB - 0.05f, z + 0.25f }, { winR - winL + 0.5f, 0.1f, 0.5f });
	window->ForEach([](SceneNode& n) { if (n.mesh) n.solid = true; });

	// ---- Sky backdrop, sun and moon (outside the window) -------------------------------------
	Material& sky = a.Mat("sky", glm::vec3(1.0f), 0.0f);
	sky.unlit = true;
	sky.plainColor=glm::vec3(0); // the procedural sky colour stays visible without its star texture
	sky.texture = a.SlotTexture(Assets::StarsSlot);
	sky.rtTextureSlot = Assets::StarsSlot;
	sky.uvScale = { 6.4f, 2.25f }; // same star size as before on a backdrop large enough for the garden view
	rig.skyMaterial = &sky;
	rig.skyCenter = { cx * 1.5f, 2.5f, -20.0f };
	room->AddShape("Sky", &plane, &sky, { cx, 38.5f, -40.0f }, { 300.0f, 1.0f, 90.0f }, { 90, 0, 0 });

	Material& sunMat = a.Mat("sun", glm::vec3(0.0f), 0.0f);
	sunMat.unlit = true;
	sunMat.emissive = { 1.0f, 0.9f, 0.55f };
	Material& moonMat = Textured(a, "moon", glm::vec3(1.65f), Assets::MoonSlot, {1, 1}, 0.0f);
	moonMat.unlit = true;
	moonMat.emissive = { 0.015f, 0.018f, 0.025f };
	rig.sun = room->AddShape("Sun", &sphere, &sunMat, rig.skyCenter, glm::vec3(2.4f));
	rig.moon = room->AddShape("Moon", &sphere, &moonMat, rig.skyCenter, glm::vec3(2.7f));
	rig.moon->local.rotation.y = -35.0f;

	// ---- Rug, poster, toy blocks ------------------------------------------------------------
	Material& rugMat = Textured(a, "rug", glm::vec3(1.0f), Assets::RugSlot, { 1.0f, 1.0f }, 0.0f);
	room->AddShape("Rug", &plane, &rugMat, { 0.5f, 0.01f, 1.5f }, { 6.0f, 1.0f, 4.5f });

	Material& posterMat = Textured(a, "poster", glm::vec3(1.0f), Assets::PosterSlot, { 1.0f, 1.0f }, 0.2f, 32.0f);
	// Keep the full printed face beside the bookcase rather than hidden behind its books.
	room->AddShape("Poster", &plane, &posterMat, { -HalfWidth + 0.02f, 3.8f, 2.2f }, { 2.0f, 1.0f, 3.0f }, { 90, 90, 0 });

	const glm::vec3 blockColors[3] = { { 0.9f, 0.2f, 0.2f }, { 0.2f, 0.4f, 0.95f }, { 1.0f, 0.8f, 0.15f } };
	// The block tower stands near the front wall, clear of the route from the chest to the door.
	const glm::vec3 blockPos[6] = { { 4.8f, 0.3f, 6.9f }, { 5.42f, 0.3f, 6.9f }, { 6.04f, 0.3f, 6.9f },
		{4.8f, 0.9f, 6.9f}, {5.42f, 0.9f, 6.9f}, {4.8f, 1.5f, 6.9f} };
	SceneNode* blocks = room->AddChild("ToyBlocks");
	Material& obstacleMat = Textured(a,"mission-crate",{0.60f,0.33f,0.13f},Assets::BlockSlot,{1,1},0.1f);
	rig.missionObstacle = solid(blocks->AddShape("DoorwayObstacle",&cube,&obstacleMat,{8.8f,1.0f,4},{1.2f,2.0f,3.0f}));
	rig.blocks.push_back(rig.missionObstacle);
	for (int i = 0; i < 6; ++i) {
		Material& m = Textured(a, "block-" + std::to_string(i), blockColors[i % 3], Assets::BlockSlot, { 1, 1 }, 0.25f, 32.0f);
		rig.blocks.push_back(solid(blocks->AddShape("Block" + std::to_string(i), &cube, &m, blockPos[i], glm::vec3(0.6f))));
	}

	// ---- Desk -------------------------------------------------------------------------------
	Material& deskMat = Textured(a, "desk-wood", { 0.85f, 0.7f, 0.6f }, Assets::FloorSlot, { 1.0f, 0.5f }, 0.3f, 32.0f);
	SceneNode* desk = room->AddChild("Desk");
	desk->local.position = { -5.0f, 0.0f, -HalfDepth + 1.4f };
	const float deskH = 2.4f, deskW = 4.0f, deskD = 2.0f, leg = 0.15f;
	desk->AddShape("Top", &cube, &deskMat, { 0, deskH, 0 }, { deskW, leg, deskD });
	for (int i = 0; i < 4; ++i) {
		const float lx = (i % 2 == 0 ? -1.0f : 1.0f) * (deskW / 2 - leg);
		const float lz = (i < 2 ? -1.0f : 1.0f) * (deskD / 2 - leg);
		desk->AddShape("Leg" + std::to_string(i), &cube, &deskMat, { lx, deskH / 2, lz }, { leg, deskH, leg });
	}
	desk->AddShape("Drawer", &cube, &deskMat, { 1.1f, deskH - 0.4f, 0.05f }, { 1.5f, 0.6f, deskD - 0.3f });
	Material& knobMat = a.Mat("brass", { 0.85f, 0.65f, 0.25f }, 0.8f, 64.0f);
	desk->AddShape("Knob", &sphere, &knobMat, { 1.1f, deskH - 0.4f, deskD / 2 - 0.12f }, glm::vec3(0.12f));
	desk->ForEach([](SceneNode& n) { if (n.mesh) n.solid = true; });
	Material& paper = a.Mat("paper", {0.88f, 0.84f, 0.72f}, 0.0f);
	solid(desk->AddShape("Sketchbook", &cube, &paper, {-0.65f, deskH + 0.12f, 0.25f}, {1.0f, 0.055f, 0.70f}, {0, 12, 0}));
	Material& pencil = a.Mat("pencil", {0.95f, 0.65f, 0.12f}, 0.2f);
	for (int i = 0; i < 3; ++i) desk->AddShape("Pencil" + std::to_string(i), &cylinder, &pencil,
		{-0.8f + i * 0.15f, deskH + 0.16f, 0.12f}, {0.025f, 0.75f, 0.025f}, {90, 12.0f + i * 8.0f, 0});

	// ---- Desk lamp (a hierarchical, haunted object) -----------------------------------------
	Material& lampMetal = a.Mat("lamp-metal", { 0.15f, 0.45f, 0.35f }, 0.7f, 64.0f);
	Material& bulb = a.Mat("bulb", glm::vec3(0.0f), 0.0f);
	bulb.unlit = true;
	bulb.emissive = { 1.0f, 0.9f, 0.6f };
	rig.bulbMaterial = &bulb;

	rig.lamp = root.AddChild("Lamp");
	rig.lamp->local.position = { -4.0f, deskH + leg / 2, -HalfDepth + 1.1f };
	rig.lamp->local.rotation = { 0, 20, 0 };
	rig.lamp->AddShape("Base", &cylinder, &lampMetal, { 0, 0.05f, 0 }, { 0.7f, 0.1f, 0.7f });
	rig.lampArm = rig.lamp->AddChild("ArmJoint");
	rig.lampArm->local.position = { 0, 0.1f, 0 };
	rig.lampArm->local.rotation = { 20, 0, 0 };
	rig.lampArm->AddShape("Arm", &cylinder, &lampMetal, { 0, 0.7f, 0 }, { 0.08f, 1.4f, 0.08f });
	rig.lampArm->AddShape("Elbow", &sphere, &lampMetal, { 0, 1.4f, 0 }, glm::vec3(0.16f));
	rig.lampHead = rig.lampArm->AddChild("HeadJoint");
	rig.lampHead->local.position = { 0, 1.4f, 0 };
	rig.lampHead->local.rotation = { -70, 0, 0 };
	rig.lampHead->AddShape("Shade", &cone, &lampMetal, { 0, -0.1f, 0 }, { 0.8f, 0.6f, 0.8f });
	rig.lampHead->AddShape("Bulb", &sphere, &bulb, { 0, -0.5f, 0 }, glm::vec3(0.22f));
	rig.lampLightAnchor = rig.lampHead->AddChild("LightAnchor");
	rig.lampLightAnchor->local.position = { 0, -0.62f, 0 };
	rig.lamp->ForEach([](SceneNode& n) { if (n.mesh && n.name != "Bulb") n.solid = true; });

	// Architectural trim, folded curtains and furnished corners give the room a lived-in scale.
	Material& trim = a.Mat("room-trim", {0.76f, 0.73f, 0.65f}, 0.2f, 24.0f);
	for (float sign : {-1.0f, 1.0f}) {
		if (sign < 0) room->AddShape("SkirtingSide", &cube, &trim, {sign * (HalfWidth - 0.06f), 0.15f, 0}, {0.12f, 0.3f, HalfDepth * 2});
		room->AddShape("SkirtingEnd", &cube, &trim, {0, 0.15f, sign * (HalfDepth - 0.06f)}, {HalfWidth * 2, 0.3f, 0.12f});
		room->AddShape("CrownSide", &cube, &trim, {sign * (HalfWidth - 0.08f), Height - 0.15f, 0}, {0.16f, 0.2f, HalfDepth * 2});
	}
	Material& curtain = Textured(a, "curtain", {0.38f, 0.45f, 0.57f}, Assets::FabricSlot, {2, 5}, 0.0f);
	for (int side = -1; side <= 1; side += 2) {
		for (int fold = 0; fold < 4; ++fold) {
			const float x = (side < 0 ? winL - 0.86f : winR + 0.14f) + fold * 0.24f;
			room->AddShape("CurtainFold", &cylinder, &curtain, {x, cy, z + 0.22f}, {0.30f, 3.6f, 0.15f});
		}
	}
	room->AddShape("CurtainRod", &cylinder, &knobMat, {cx, winT + 0.4f, z + 0.23f}, {0.055f, 6.3f, 0.055f}, {0, 0, 90});
	// Orange plaid bed sheet with a beige pillow, as in the photos of Penny asleep.
	Material& quilt = Textured(a, "quilt", {1.0f, 0.62f, 0.30f}, Assets::PlaidSlot, {3, 4}, 0.0f);
	quilt.plainColor = {0.86f, 0.48f, 0.20f};
	SceneNode* bed = room->AddChild("Bed");
	bed->local.position = {7.2f, 0, -5.8f};
	solid(bed->AddShape("BedFrame", &cube, &deskMat, {0, 0.45f, 0}, {3.4f, 0.75f, 4.7f}));
	solid(bed->AddShape("Mattress", &cube, &paper, {0, 0.94f, 0}, {3.3f, 0.3f, 4.55f}));
	solid(bed->AddShape("Blanket", &cube, &quilt, {0, 1.10f, 0.55f}, {3.36f, 0.12f, 3.45f}));
	solid(bed->AddShape("Headboard", &cube, &deskMat, {0, 1.0f, -2.35f}, {3.6f, 1.8f, 0.18f}));
	for (float side : {-1.0f, 1.0f}) solid(bed->AddShape("Pillow", &sphere, &paper, {0.8f * side, 1.2f, -1.5f}, {1.4f, 0.3f, 0.9f}));
	SceneNode* shelf = room->AddChild("Bookcase");
	Material& books = Textured(a, "book-spines", glm::vec3(1.0f), Assets::BookSlot, {1, 1}, 0.08f, 16.0f);
	books.plainColor = {0.42f, 0.22f, 0.16f};
	shelf->local.position = {-8.9f, 0, -1.5f};
	for (float side : {-1.0f, 1.0f}) solid(shelf->AddShape("Upright", &cube, &deskMat, {side * 0.9f, 1.75f, 0}, {0.12f, 3.5f, 1.0f}));
	solid(shelf->AddShape("Back", &cube, &deskMat, {0, 1.75f, -0.47f}, {1.8f, 3.5f, 0.08f}));
	for (int tier = 0; tier < 4; ++tier) {
		const float level = 0.15f + tier * 1.05f;
		solid(shelf->AddShape("Shelf", &cube, &deskMat, {0, level, 0}, {1.8f, 0.09f, 1.0f}));
		// One box per shelf whose front shows a row of book spines (a texture) instead of 6 book cubes.
		solid(shelf->AddShape("Books", &cube, &books, {0, level + 0.37f, -0.1f}, {1.56f, 0.66f, 0.56f}));
	}
	// Distant silhouettes are outside the sealed play space and visible through the window.
	Material& silhouette = a.Mat("distant-roofs", {0.035f, 0.045f, 0.085f}, 0.0f);
	silhouette.unlit = true;
	rig.skylineMaterial = &silhouette;
	for (int i = 0; i < 9; ++i) {
		const float h = 7.5f + static_cast<float>(i % 3) * 1.5f; // stand on the garden level, tops show through the window
		room->AddShape("DistantHouse", &cube, &silhouette, {-22.0f + i * 6.0f, Ground + h * 0.5f, -29.0f}, {5.0f, h, 2.0f});
	}

	// Ceiling fan: the shaft is stationary, and one rotor joint owns all four blades.
	SceneNode* fan = room->AddChild("CeilingFan");
	fan->local.position = {-1.5f, Height - 0.08f, 0.0f};
	fan->AddShape("Mount", &cylinder, &lampMetal, {0,-0.06f,0}, {0.36f,0.12f,0.36f});
	fan->AddShape("Shaft", &cylinder, &knobMat, {0,-0.35f,0}, {0.08f,0.55f,0.08f});
	rig.fanRotor = fan->AddChild("Rotor");
	rig.fanRotor->local.position = {0,-0.63f,0};
	rig.fanRotor->AddShape("Motor", &sphere, &lampMetal, {0,0,0}, {0.5f,0.25f,0.5f});
	for (int i=0;i<4;++i) {
		SceneNode* blade = rig.fanRotor->AddChild("BladePivot" + std::to_string(i));
		blade->local.rotation.y = i * 90.0f;
		blade->AddShape("Blade", &cube, &deskMat, {0.83f,0,0}, {1.35f,0.045f,0.30f}, {0,0,-4});
	}
	// Desk chair: seat/back use wood, with four legs and a back joint.
	SceneNode* chair = room->AddChild("DeskChair");
	chair->local.position = {-5.6f,0,-4.8f};
	solid(chair->AddShape("Seat", &cube, &deskMat, {0,1.15f,0}, {1.15f,0.12f,1.05f}));
	solid(chair->AddShape("Back", &cube, &deskMat, {0,1.9f,0.47f}, {1.15f,1.35f,0.12f}));
	for (float x : {-0.43f,0.43f}) for (float zc : {-0.38f,0.38f})
		solid(chair->AddShape("Leg", &cube, &deskMat, {x,0.55f,zc}, {0.10f,1.1f,0.10f}));
	// Clock lies on the back wall; hands rotate around local Z in the face plane.
	SceneNode* clock = room->AddChild("WallClock");
	clock->local.position = {-1.2f,5.1f,-HalfDepth+0.16f};
	clock->AddShape("Rim", &cylinder, &knobMat, {0,0,0}, {1.05f,0.12f,1.05f}, {90,0,0});
	clock->AddShape("Face", &cylinder, &paper, {0,0,0.07f}, {0.94f,0.03f,0.94f}, {90,0,0});
	Material& ink = a.Mat("clock-ink", {0.05f,0.06f,0.09f},0.0f);
	for (int i=0;i<12;++i) {
		const float angle=glm::radians(i*30.0f);
		clock->AddShape("HourMark", &cube, &ink, {0.39f*std::sin(angle),0.39f*std::cos(angle),0.095f}, {0.035f,0.065f,0.015f}, {0,0,-i*30.0f});
	}
	rig.clockHour=clock->AddChild("HourHandPivot");
	rig.clockMinute=clock->AddChild("MinuteHandPivot");
	rig.clockHour->AddShape("HourHand", &cube, &ink, {0,0.12f,0.115f},{0.045f,0.27f,0.02f});
	rig.clockMinute->AddShape("MinuteHand", &cube, &ink, {0,0.18f,0.14f},{0.026f,0.38f,0.02f});
	clock->AddShape("Pin", &sphere, &knobMat, {0,0,0.17f},glm::vec3(0.075f));

	// ---- Beach ball ------------------------------------------------------------------------
	Material& ballMat = Textured(a, "beach-ball", glm::vec3(1.0f), Assets::BallSlot, { 1, 1 }, 0.6f, 64.0f);
	ballMat.reflectivity = 0.12f;
	rig.ball = root.AddChild("Ball");
	rig.ball->local.position = { -3.5f, rig.ballRadius, 3.5f };
	rig.ballShape = rig.ball->AddShape("BallShape", &sphere, &ballMat, { 0, 0, 0 }, glm::vec3(2 * rig.ballRadius));

	// ---- Ghost ---------------------------------------------------------------------------
	Material& ghostMat = a.Mat("ghost", { 0.9f, 0.95f, 1.0f }, 0.2f, 16.0f);
	ghostMat.emissive = { 0.15f, 0.2f, 0.3f };
	ghostMat.opacity = 0.55f;
	rig.ghostMaterial = &ghostMat;
	Material& ghostEyes = a.Mat("ghost-eyes", { 0.02f, 0.02f, 0.05f }, 0.5f, 32.0f);
	rig.ghost = root.AddChild("Ghost");
	rig.ghost->local.position = { 0, 4.0f, -1.0f };
	rig.ghostBody = rig.ghost->AddChild("Body");
	rig.ghostBody->AddShape("Head", &sphere, &ghostMat, { 0, 0.35f, 0 }, glm::vec3(0.9f));
	rig.ghostBody->AddShape("Sheet", &cone, &ghostMat, { 0, -0.35f, 0 }, { 1.1f, 1.3f, 1.1f });
	rig.ghostBody->AddShape("EyeL", &sphere, &ghostEyes, { -0.16f, 0.45f, 0.38f }, { 0.14f, 0.2f, 0.1f });
	rig.ghostBody->AddShape("EyeR", &sphere, &ghostEyes, { 0.16f, 0.45f, 0.38f }, { 0.14f, 0.2f, 0.1f });
	rig.ghostBody->AddShape("Mouth", &sphere, &ghostEyes, { 0, 0.22f, 0.42f }, { 0.12f, 0.16f, 0.08f });

	return rig;
}
