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

	SceneNode* room = root.AddChild("Room");

	// ---- Floor and ceiling ------------------------------------------------------------------
	Material& floorMat = Textured(a, "floor", glm::vec3(1.0f), Assets::FloorSlot, { 4.0f, 3.0f }, 0.35f, 48.0f);
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
	AddWall(*room, "RightWall", plane, wallMat("wall-side", { 2 * HalfDepth, Height }), { HalfWidth, wy, 0 }, -90.0f, { 2 * HalfDepth, Height });
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

	// ---- Sky backdrop, sun and moon (outside the window) -------------------------------------
	Material& sky = a.Mat("sky", glm::vec3(1.0f), 0.0f);
	sky.unlit = true;
	sky.texture = a.SlotTexture(Assets::StarsSlot);
	sky.rtTextureSlot = Assets::StarsSlot;
	sky.uvScale = { 3.0f, 1.0f };
	rig.skyMaterial = &sky;
	rig.skyCenter = { cx, 2.0f, -34.0f };
	room->AddShape("Sky", &plane, &sky, { cx, 18.0f, -40.0f }, { 140.0f, 1.0f, 40.0f }, { 90, 0, 0 });

	Material& sunMat = a.Mat("sun", glm::vec3(0.0f), 0.0f);
	sunMat.unlit = true;
	sunMat.emissive = { 1.0f, 0.9f, 0.55f };
	Material& moonMat = a.Mat("moon", glm::vec3(0.0f), 0.0f);
	moonMat.unlit = true;
	moonMat.emissive = { 0.85f, 0.88f, 1.0f };
	rig.sun = room->AddShape("Sun", &sphere, &sunMat, rig.skyCenter, glm::vec3(4.0f));
	rig.moon = room->AddShape("Moon", &sphere, &moonMat, rig.skyCenter, glm::vec3(3.0f));

	// ---- Rug, poster, toy blocks ------------------------------------------------------------
	Material& rugMat = Textured(a, "rug", glm::vec3(1.0f), Assets::RugSlot, { 1.0f, 1.0f }, 0.0f);
	room->AddShape("Rug", &plane, &rugMat, { 0.5f, 0.01f, 1.5f }, { 6.0f, 1.0f, 4.5f });

	Material& posterMat = Textured(a, "poster", glm::vec3(1.0f), Assets::PosterSlot, { 1.0f, 1.0f }, 0.2f, 32.0f);
	room->AddShape("Poster", &plane, &posterMat, { -HalfWidth + 0.02f, 3.8f, -1.0f }, { 2.0f, 1.0f, 3.0f }, { 90, 90, 0 });

	const glm::vec3 blockColors[3] = { { 0.9f, 0.2f, 0.2f }, { 0.2f, 0.4f, 0.95f }, { 1.0f, 0.8f, 0.15f } };
	const glm::vec3 blockPos[3] = { { 5.2f, 0.3f, 3.8f }, { 5.9f, 0.3f, 3.6f }, { 5.55f, 0.9f, 3.7f } };
	const float blockYaw[3] = { 10.0f, -15.0f, 30.0f };
	SceneNode* blocks = room->AddChild("ToyBlocks");
	for (int i = 0; i < 3; ++i) {
		Material& m = Textured(a, "block-" + std::to_string(i), blockColors[i], Assets::BlockSlot, { 1, 1 }, 0.3f, 32.0f);
		blocks->AddShape("Block" + std::to_string(i), &cube, &m, blockPos[i], glm::vec3(0.6f), { 0, blockYaw[i], 0 });
	}

	// ---- Desk -------------------------------------------------------------------------------
	Material& deskMat = Textured(a, "desk-wood", { 0.85f, 0.7f, 0.6f }, Assets::FloorSlot, { 1.0f, 0.5f }, 0.3f, 32.0f);
	SceneNode* desk = room->AddChild("Desk");
	desk->local.position = { -5.0f, 0.0f, -4.6f };
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

	// ---- Desk lamp (a hierarchical, haunted object) -----------------------------------------
	Material& lampMetal = a.Mat("lamp-metal", { 0.15f, 0.45f, 0.35f }, 0.7f, 64.0f);
	Material& bulb = a.Mat("bulb", glm::vec3(0.0f), 0.0f);
	bulb.unlit = true;
	bulb.emissive = { 1.0f, 0.9f, 0.6f };
	rig.bulbMaterial = &bulb;

	rig.lamp = root.AddChild("Lamp");
	rig.lamp->local.position = { -4.0f, deskH + leg / 2, -4.9f };
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
