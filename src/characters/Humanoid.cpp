#include "Humanoid.h"

#include <algorithm>
#include <cmath>

#include "render/Assets.h"
#include "scene/SceneNode.h"
#include "world/PhysicsWorld.h"

namespace {

// Builds one leg under a hip joint: the joint sits at the hip, the leg hangs down its local -Y,
// so rotating the joint about X swings the whole leg (leg + boot + foot) like a pendulum.
void BuildLeg(SceneNode* hip, Assets& a, const Material& pants, const Material& boots)
{
	hip->AddShape("Leg", &a.Cylinder(), &pants, { 0, -0.31f, 0 }, { 0.15f, 0.62f, 0.15f });
	hip->AddShape("BootShaft", &a.Cylinder(), &boots, { 0, -0.67f, 0 }, { 0.18f, 0.22f, 0.18f });
	hip->AddShape("Foot", &a.Cube(), &boots, { 0, -0.80f, 0.05f }, { 0.18f, 0.10f, 0.30f });
	hip->AddShape("Knee", &a.Sphere(), &pants, {0, -0.34f, 0.035f}, {0.155f, 0.16f, 0.13f});
	Material& sole = a.Mat("boot-sole", {0.12f, 0.08f, 0.055f}, 0.2f, 12);
	hip->AddShape("Sole", &a.Cube(), &sole, {0, -0.848f, 0.055f}, {0.19f, 0.025f, 0.32f});
	hip->AddShape("RoundedToe", &a.Sphere(), &boots, {0, -0.78f, 0.16f}, {0.18f, 0.09f, 0.13f});
}

SceneNode* BuildArm(SceneNode* shoulder, Assets& a, const Material& sleeve, const Material& hand)
{
	shoulder->AddShape("UpperArm", &a.Cylinder(), &sleeve, { 0, -0.14f, 0 }, { 0.12f, 0.28f, 0.12f });
	shoulder->AddShape("ShoulderBall", &a.Sphere(), &sleeve, { 0, 0, 0 }, glm::vec3(0.13f));
	shoulder->AddShape("Elbow", &a.Sphere(), &sleeve, {0, -0.29f, 0}, glm::vec3(0.12f));
	shoulder->AddShape("Forearm", &a.Cylinder(), &sleeve, {0, -0.41f, 0}, {0.105f, 0.23f, 0.105f});
	shoulder->AddShape("Cuff", &a.Cylinder(), &sleeve, {0, -0.50f, 0}, {0.125f, 0.06f, 0.125f});
	SceneNode* palm = shoulder->AddShape("Hand", &a.Sphere(), &hand, { 0, -0.57f, 0 }, {0.13f, 0.14f, 0.09f});
	// The four fingers are one rounded block (a toy's mitten hand): one shape instead of four cylinders.
	shoulder->AddShape("Fingers", &a.Sphere(), &hand, {-0.002f, -0.63f, 0.022f}, {0.12f, 0.09f, 0.06f});
	shoulder->AddShape("Thumb", &a.Sphere(), &hand, {0.068f, -0.565f, 0.025f}, {0.06f, 0.065f, 0.05f});
	return palm;
}

} // namespace

HumanoidStyle WoodyStyle()
{
	HumanoidStyle s;
	s.name = "Woody";
	s.shirt = { 0.95f, 0.85f, 0.35f };  // yellow plaid shirt
	s.vest = { 0.95f, 0.95f, 0.90f };   // white cow-print vest
	s.pants = { 0.20f, 0.35f, 0.75f };  // blue jeans
	s.boots = { 0.45f, 0.25f, 0.10f };
	s.hair = { 0.35f, 0.20f, 0.08f };
	s.hat = { 0.55f, 0.35f, 0.18f };
	return s;
}

HumanoidStyle JessieStyle()
{
	HumanoidStyle s;
	s.name = "Jessie";
	s.shirt = { 0.97f, 0.97f, 0.95f };  // white shirt
	s.vest = { 0.85f, 0.15f, 0.15f };   // red yoke
	s.pants = { 0.25f, 0.45f, 0.85f };  // blue jeans
	s.boots = { 0.55f, 0.35f, 0.20f };
	s.hair = { 0.85f, 0.25f, 0.08f };   // red hair
	s.hat = { 0.85f, 0.12f, 0.12f };    // red hat
	s.longHair = true;
	return s;
}

Humanoid::Humanoid(SceneNode& parent, Assets& a, const HumanoidStyle& st, const glm::vec3& position, float headingDeg)
	: Character(st.name, parent.AddChild(st.name))
{
	root->local.position = position;
	root->local.rotation.y = headingDeg;
	maxSpeed = 1.8f;

	const std::string n = st.name + "-";
	Material& skin = a.Mat(n + "skin", st.skin, 0.2f, 16.0f);
	Material& shirt = a.Mat(n + "shirt", st.shirt, 0.15f, 12.0f);
	Material& vest = a.Mat(n + "vest", st.vest, 0.15f, 12.0f);
	Material& pants = a.Mat(n + "pants", st.pants, 0.1f, 8.0f);
	Material& boots = a.Mat(n + "boots", st.boots, 0.6f, 48.0f);
	Material& hands = a.Mat(n + "hands", st.hands, 0.2f, 16.0f);
	Material& hair = a.Mat(n + "hair", st.hair, 0.3f, 24.0f);
	Material& hat = a.Mat(n + "hat", st.hat, 0.2f, 16.0f);
	Material& belt = a.Mat(n + "belt", st.belt, 0.4f, 32.0f);
	Material& white = a.Mat("eye-white", { 1.0f, 1.0f, 1.0f }, 0.8f, 96.0f);
	Material& pupil = a.Mat("eye-pupil", { 0.05f, 0.05f, 0.08f }, 0.9f, 128.0f);
	Material& mouth = a.Mat("mouth", { 0.55f, 0.1f, 0.12f }, 0.2f, 16.0f);
	Material& brass = a.Mat("brass", { 0.85f, 0.65f, 0.25f }, 0.8f, 64.0f);
	auto surface = [&](Material& m, int slot, glm::vec2 repeats) {
		m.texture = a.SlotTexture(slot); m.rtTextureSlot = slot; m.uvScale = repeats;
	};
	if (!st.spaceRanger) {
		surface(shirt, st.longHair ? Assets::FabricSlot : Assets::PlaidSlot, {1, 1});
		surface(vest, st.longHair ? Assets::FabricSlot : Assets::CowSlot, {1, 1});
		surface(pants, Assets::DenimSlot, {2, 2});
		surface(boots, Assets::LeatherSlot, {1, 1});
		surface(hat, Assets::LeatherSlot, {1, 1});
		surface(belt, Assets::LeatherSlot, {1, 1});
	}

	const Mesh& cube = a.Cube();
	const Mesh& sphere = a.Sphere();
	const Mesh& cylinder = a.Cylinder();

	// Pelvis (joint at hip height) --------------------------------------------------------
	SceneNode* pelvis = root->AddChild("Pelvis");
	pelvis->local.position = { 0, HipHeight, 0 };
	pelvis->AddShape("Belt", &cube, &belt, { 0, 0.03f, 0 }, { 0.40f, 0.12f, 0.25f });
	pelvis->AddShape("Buckle", &cube, &brass, { 0, 0.03f, 0.13f }, { 0.11f, 0.08f, 0.03f });

	// Legs
	leftHip = pelvis->AddChild("LeftHip");
	leftHip->local.position = { 0.11f, 0, 0 };
	BuildLeg(leftHip, a, pants, boots);
	rightHip = pelvis->AddChild("RightHip");
	rightHip->local.position = { -0.11f, 0, 0 };
	BuildLeg(rightHip, a, pants, boots);

	// Torso ------------------------------------------------------------------------------
	torso = pelvis->AddChild("Torso");
	torso->local.position = { 0, 0.09f, 0 };
	torso->AddShape("Chest", &cube, &shirt, { 0, 0.25f, 0 }, { 0.42f, 0.5f, 0.24f });
	if (st.hasVest) {
		torso->AddShape("VestLeft", &cube, &vest, { 0.13f, 0.27f, 0.125f }, { 0.15f, 0.44f, 0.02f });
		torso->AddShape("VestRight", &cube, &vest, { -0.13f, 0.27f, 0.125f }, { 0.15f, 0.44f, 0.02f });
		torso->AddShape("VestBack", &cube, &vest, { 0, 0.27f, -0.125f }, { 0.43f, 0.44f, 0.02f });
	}
	torso->AddShape("Neck", &cylinder, &skin, { 0, 0.54f, 0 }, { 0.1f, 0.1f, 0.1f });
	if (!st.spaceRanger) {
		Material& collar = a.Mat(n + "collar", st.longHair ? glm::vec3(0.95f, 0.69f, 0.22f) : st.shirt, 0.06f);
		for (float side : {-1.0f, 1.0f}) {
			torso->AddShape("Collar", &cube, &collar, {side * 0.075f, 0.47f, 0.14f}, {0.13f, 0.1f, 0.025f}, {0, 0, side * 24});
		}
		Material& scarf = a.Mat(n + "scarf", {0.67f, 0.09f, 0.075f}, 0.04f);
		torso->AddShape("Neckerchief", &cylinder, &scarf, {0, 0.53f, 0}, {0.17f, 0.055f, 0.17f});
		torso->AddShape("ScarfKnot", &sphere, &scarf, {0, 0.5f, 0.13f}, glm::vec3(0.065f));
		torso->AddShape("ScarfTail", &a.Cone(), &scarf, {0.025f, 0.40f, 0.155f}, {0.065f, 0.18f, 0.015f}, {0, 0, 12});
		if (!st.longHair) {
			torso->AddShape("SheriffBadge", &cylinder, &brass, {0.14f, 0.39f, 0.145f}, {0.09f, 0.015f, 0.09f}, {90, 0, 0});
			pelvis->AddShape("Holster", &cube, &belt, {-0.24f, -0.10f, 0}, {0.09f, 0.23f, 0.15f}, {0, 0, -12});
		}
	}

	// Arms (shoulder joints hang the arm along -Y, slightly outward)
	leftShoulder = torso->AddChild("LeftShoulder");
	leftShoulder->local.position = { 0.27f, 0.46f, 0 };
	BuildArm(leftShoulder, a, shirt, hands);
	rightShoulder = torso->AddChild("RightShoulder");
	rightShoulder->local.position = { -0.27f, 0.46f, 0 };
	rightHand = BuildArm(rightShoulder, a, shirt, hands);

	// Head -------------------------------------------------------------------------------
	head = torso->AddChild("Head");
	head->local.position = { 0, 0.56f, 0 };
	head->AddShape("Skull", &sphere, &skin, { 0, 0.2f, 0 }, { 0.34f, 0.38f, 0.34f });
	for (int side = -1; side <= 1; side += 2) {
		const float x = 0.07f * static_cast<float>(side);
		head->AddShape(side < 0 ? "EyeRight" : "EyeLeft", &sphere, &white, { x, 0.24f, 0.145f }, { 0.08f, 0.09f, 0.05f });
		head->AddShape(side < 0 ? "PupilRight" : "PupilLeft", &sphere, &pupil, { x, 0.24f, 0.165f }, { 0.04f, 0.05f, 0.03f });
	}
	head->AddShape("Nose", &sphere, &skin, { 0, 0.18f, 0.17f }, glm::vec3(0.055f));
	head->AddShape("Mouth", &cube, &mouth, { 0, 0.1f, 0.15f }, { 0.1f, 0.02f, 0.03f });
	Material& iris = a.Mat(n + "iris", st.longHair ? glm::vec3(0.18f, 0.45f, 0.23f) : glm::vec3(0.32f, 0.23f, 0.10f), 0.65f, 72);
	for (float side : {-1.0f, 1.0f}) {
		head->AddShape("Ear", &sphere, &skin, {side * 0.17f, 0.20f, 0}, {0.07f, 0.11f, 0.055f});
		head->AddShape("Iris", &sphere, &iris, {side * 0.07f, 0.24f, 0.168f}, {0.046f, 0.052f, 0.014f});
		head->AddShape("Eyebrow", &cube, &hair, {side * 0.072f, 0.303f, 0.146f}, {0.079f, 0.016f, 0.019f}, {0, 0, side * 9});
		head->AddShape("Cheek", &sphere, &skin, {side * 0.095f, 0.14f, 0.12f}, {0.09f, 0.075f, 0.06f});
	}
	head->AddShape("ChinDetail", &sphere, &skin, {0, 0.04f, 0.11f}, {0.17f, 0.075f, 0.09f});

	if (!st.spaceRanger) {
		head->AddShape("Hair", &sphere, &hair, { 0, 0.27f, -0.035f }, { 0.36f, 0.32f, 0.35f });
		if (st.longHair) {
			SceneNode* braid = head->AddChild("Braid");
			braid->local.position = { 0, 0.22f, -0.17f };
			braid->local.rotation = { -12.0f, 0, 0 };
			// The woven texture shows the plait pattern on one cylinder (it used to be 12 spheres).
			Material& plait = a.Mat(n + "plait", st.hair, 0.3f, 24.0f);
			surface(plait, Assets::FabricSlot, {2, 6});
			braid->AddShape("Plait", &cylinder, &plait, { 0, -0.28f, 0 }, { 0.12f, 0.56f, 0.12f });
			braid->AddShape("Bow", &sphere, &a.Mat("hair-bow", { 1.0f, 0.85f, 0.2f }), { 0, -0.02f, -0.02f }, { 0.16f, 0.09f, 0.09f });
			braid->AddShape("Tip", &sphere, &hair, { 0, -0.58f, 0 }, glm::vec3(0.12f));
		}
		if (st.hasHat) {
			head->AddShape("HatBrim", &cylinder, &hat, { 0, 0.38f, 0 }, { 0.66f, 0.03f, 0.66f });
			head->AddShape("HatCrown", &cylinder, &hat, { 0, 0.49f, 0 }, { 0.30f, 0.2f, 0.30f });
			head->AddShape("HatBand", &cylinder, &belt, { 0, 0.42f, 0 }, { 0.31f, 0.04f, 0.31f });
		}
	}
	SaveHome();
}

void Humanoid::Animate(float dt, float time)
{
	Character::Animate(dt, time);

	seatBlend += ((seated ? 1.0f : 0.0f) - seatBlend) * std::min(1.0f, dt * 6.0f);

	// Walk cycle: legs swing in opposite phase, arms swing opposite to the legs.
	const float swing = std::sin(walkPhase) * 35.0f * moveBlend * (1.0f - seatBlend) * groundMotion;

	leftHip->local.rotation = glm::mix(glm::vec3(swing, 0, 0), glm::vec3(-40, 0, 32), seatBlend);
	rightHip->local.rotation = glm::mix(glm::vec3(-swing, 0, 0), glm::vec3(-40, 0, -32), seatBlend);

	const glm::vec3 leftArm = glm::mix(glm::vec3(-swing * 0.8f, 0, 6), glm::vec3(-50, 0, 8), seatBlend);
	glm::vec3 rightArm = glm::mix(glm::vec3(swing * 0.8f, 0, -6), glm::vec3(-50, 0, -8), seatBlend);
	rightArm = glm::mix(rightArm, glm::vec3(-90 + laserPitch, 0, 0), rightArmOverride);
	// Gestures blend on top of walking and riding.
	const float k = std::min(1.0f, dt * 6.0f);
	reachBlend += ((reach ? 1.0f : 0.0f) - reachBlend) * k;
	cheerTime = std::max(0.0f, cheerTime - dt);
	cheerBlend += ((cheerTime > 0.0f ? 1.0f : 0.0f) - cheerBlend) * k;
	const float wave = std::sin(time * 9.0f) * 18.0f;
	glm::vec3 left = glm::mix(leftArm, glm::vec3(-10, 0, 150 + wave), cheerBlend);
	rightArm = glm::mix(rightArm, glm::vec3(-10, 0, -150 - wave), cheerBlend);
	rightArm = glm::mix(rightArm, glm::vec3(-25, 0, -140), reachBlend);
	leftShoulder->local.rotation = left;
	rightShoulder->local.rotation = rightArm;

	// Body bob while walking, idle "looking around" when standing still.
	torso->Parent()->local.position.y = HipHeight + std::abs(std::sin(walkPhase)) * 0.04f * moveBlend * groundMotion;
	const float idle = 1.0f - moveBlend;
	head->local.rotation.y = std::sin(time * 0.6f + static_cast<float>(name.size())) * 25.0f * idle;
	head->local.rotation.x = std::sin(time * 0.9f + 1.3f) * 6.0f * idle;
}

// ------------------------------------------------------------------------------------------
// Buzz
// ------------------------------------------------------------------------------------------
namespace {
HumanoidStyle BuzzStyle()
{
	HumanoidStyle s;
	s.name = "Buzz";
	s.shirt = { 0.95f, 0.95f, 0.95f };  // white suit
	s.vest = { 0.35f, 0.75f, 0.25f };   // green side panels
	s.pants = { 0.92f, 0.92f, 0.92f };
	s.boots = { 0.50f, 0.30f, 0.70f };  // purple
	s.hands = { 0.50f, 0.30f, 0.70f };
	s.belt = { 0.35f, 0.75f, 0.25f };
	s.hasHat = false;
	s.spaceRanger = true;
	return s;
}
}

Buzz::Buzz(SceneNode& parent, Assets& a, const glm::vec3& position, float headingDeg)
	: Humanoid(parent, a, BuzzStyle(), position, headingDeg)
{
	canFly = true;
	const Mesh& cube = a.Cube();
	const Mesh& sphere = a.Sphere();
	const Mesh& cylinder = a.Cylinder();

	Material& purple = a.Mat("Buzz-hood", { 0.50f, 0.30f, 0.70f }, 0.3f, 24.0f);
	Material& green = a.Mat("Buzz-vest", { 0.35f, 0.75f, 0.25f });
	Material& white = a.Mat("Buzz-shirt", { 0.95f, 0.95f, 0.95f });
	Material& red = a.Mat("Buzz-red", { 0.9f, 0.15f, 0.12f }, 0.6f, 64.0f);
	Material& blue = a.Mat("Buzz-blue", { 0.2f, 0.45f, 0.95f }, 0.6f, 64.0f);
	Material& glass = a.Mat("Buzz-helmet", { 0.8f, 0.9f, 1.0f }, 1.0f, 128.0f);
	glass.opacity = 0.22f;
	glass.reflectivity = 0.25f;

	// Hood behind the face, transparent helmet around the head
	head->AddShape("Hood", &sphere, &purple, { 0, 0.23f, -0.05f }, { 0.37f, 0.40f, 0.33f });
	head->AddShape("Chin", &sphere, &purple, { 0, 0.03f, 0.02f }, { 0.28f, 0.1f, 0.26f });
	head->AddShape("Helmet", &sphere, &glass, { 0, 0.21f, 0.02f }, glm::vec3(0.6f));

	// Chest plate with buttons
	torso->AddShape("ChestPlate", &cube, &white, { 0, 0.33f, 0.13f }, { 0.34f, 0.24f, 0.05f });
	torso->AddShape("ChestStripe", &cube, &purple, { 0, 0.2f, 0.135f }, { 0.34f, 0.04f, 0.05f });
	torso->AddShape("ButtonRed", &sphere, &red, { -0.08f, 0.33f, 0.16f }, glm::vec3(0.05f));
	torso->AddShape("ButtonGreen", &sphere, &green, { 0, 0.33f, 0.16f }, glm::vec3(0.05f));
	torso->AddShape("ButtonBlue", &sphere, &blue, { 0.08f, 0.33f, 0.16f }, glm::vec3(0.05f));
	Material& dark = a.Mat("Buzz-joints", {0.12f, 0.14f, 0.18f}, 0.4f, 40);
	torso->AddShape("RangerBadge", &cube, &green, {0.08f, 0.42f, 0.17f}, {0.14f, 0.043f, 0.015f});
	for (SceneNode* shoulder : {leftShoulder, rightShoulder}) {
		shoulder->AddShape("ShoulderArmor", &sphere, &green, {0, -0.04f, 0}, {0.18f, 0.16f, 0.18f});
		shoulder->AddShape("WristBand", &cylinder, &green, {0, -0.47f, 0}, {0.14f, 0.08f, 0.14f});
	}
	for (SceneNode* hip : {leftHip, rightHip}) {
		hip->AddShape("KneeArmor", &sphere, &green, {0, -0.34f, 0.06f}, {0.16f, 0.16f, 0.12f});
	}

	// Wings: a joint on the back whose X scale opens / folds both wings
	wings = torso->AddChild("Wings");
	wings->local.position = { 0, 0.33f, -0.16f };
	wings->AddShape("Pack", &cube, &white, { 0, 0, 0 }, { 0.3f, 0.3f, 0.1f });
	for (float side : {-1.0f, 1.0f}) {
		wings->AddShape("Thruster", &cylinder, &dark, {side * 0.12f, -0.18f, -0.03f}, {0.10f, 0.13f, 0.10f});
		wings->AddShape("ThrusterRim", &cylinder, &purple, {side * 0.12f, -0.24f, -0.03f}, {0.12f, 0.03f, 0.12f});
	}
	for (int side = -1; side <= 1; side += 2) {
		const float s = static_cast<float>(side);
		wings->AddShape(side < 0 ? "WingRight" : "WingLeft", &cube, &white, { 0.45f * s, 0.02f, -0.02f }, { 0.75f, 0.06f, 0.22f }, { 0, 0, 8.0f * s });
		wings->AddShape(side < 0 ? "WingTipRight" : "WingTipLeft", &cube, &red, { 0.85f * s, 0.08f, -0.02f }, { 0.1f, 0.07f, 0.23f }, { 0, 0, 8.0f * s });
	}

	// Wrist laser + beam (beam along the arm's -Y, i.e. out of the hand)
	Material& beam = a.Mat("laser-beam", { 0.0f, 0.0f, 0.0f }, 0.0f);
	beam.unlit = true;
	beam.emissive = { 1.0f, 0.1f, 0.1f };
	beam.opacity = 0.85f;
	rightShoulder->AddShape("LaserEmitter", &cylinder, &red, { 0, -0.42f, 0.05f }, { 0.07f, 0.12f, 0.07f });
	laserBeam = rightShoulder->AddShape("LaserBeam", &cylinder, &beam, { 0, -3.6f, 0 }, { 0.035f, 6.0f, 0.035f });
	laserBeam->visible = false;
	laserTip = rightShoulder->AddChild("LaserTip");
	laserTip->local.position = { 0, -0.7f, 0 };
	previousFlightPosition = position;
	previousFlightHeading = headingDeg;
	SaveHome(); // include wings and laser joints in the saved landing/rest pose
}

glm::vec3 Buzz::LaserTip() const
{
	return laserTip->WorldPosition();
}

glm::vec3 Buzz::LaserDirection() const
{
	return glm::normalize(glm::vec3(rightShoulder->World() * glm::vec4(0, -1, 0, 0)));
}
void Buzz::SetLaserLength(float length)
{
	laserBeam->local.position.y = -0.7f - length * 0.5f;
	laserBeam->local.scale.y = std::max(0.001f, length);
}
void Buzz::TiltLaser(float degrees) { laserPitch = std::clamp(laserPitch + degrees, -65.0f, 65.0f); }
void Buzz::AimAt(const glm::vec3& point)
{
	const glm::vec3 delta = point - LaserTip();
	if (glm::length(delta) < 0.01f) return;
	root->local.rotation.y = glm::degrees(std::atan2(delta.x, delta.z));
	laserPitch = std::clamp(-glm::degrees(std::atan2(delta.y, glm::length(glm::vec2(delta.x, delta.z)))), -65.0f, 65.0f);
}

void Buzz::Animate(float dt, float time)
{
	const float k = 1.0f - std::exp(-dt * 8.0f);
	// Flying = above the floor under him (upper floor, ground floor, a stair tread or the garden).
	const bool flying = root->local.position.y > PhysicsWorld::FloorHeight(root->local.position) + 0.06f;
	const glm::vec3 displacement = root->local.position - previousFlightPosition;
	// Use actual movement: climbing and hovering must not look like running.
	const glm::vec3 velocity = dt > 0.0001f && glm::length(displacement) < 1.0f
		? displacement / dt : glm::vec3(0);
	const float horizontalSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
	const float cruise = flying ? std::clamp(horizontalSpeed / 1.8f, 0.0f, 1.0f) : 0.0f;
	flightBlend += ((flying ? 1.0f : 0.0f) - flightBlend) * k;
	cruiseBlend += (cruise - cruiseBlend) * k;
	groundMotion = flying ? 0.0f : 1.0f - flightBlend;
	const float headingVelocity = dt > 0.0001f ? WrapDegrees(Heading() - previousFlightHeading) / dt : 0.0f;
	previousFlightPosition = root->local.position;
	previousFlightHeading = Heading();
	rightArmOverride += ((laserOn ? 1.0f : 0.0f) - rightArmOverride) * k;
	Humanoid::Animate(dt, time);

	laserBeam->visible = laserOn && rightArmOverride > 0.9f;

	// A separate flight rig: both legs trail together, arms brace, body leans and banks.
	// Hover is upright; moving flight is pitched forward. None of this uses walkPhase.
	wingOpen += ((flying ? 1.0f : 0.0f) - wingOpen) * k;
	wings->local.scale = { 0.08f + 0.92f * wingOpen, 1.0f, 1.0f };
	const float climbTilt = std::clamp(velocity.y * 7.0f, -12.0f, 12.0f);
	const float targetPitch = flightBlend * (laserOn ? 8.0f : 6.0f + 44.0f * cruiseBlend - climbTilt);
	const float targetBank = flightBlend * (laserOn ? 0.0f : std::clamp(-headingVelocity * 0.12f, -14.0f, 14.0f));
	flightPitch += (targetPitch - flightPitch) * k;
	flightBank += (targetBank - flightBank) * k;
	SceneNode* pelvis = torso->Parent();
	pelvis->local.rotation = {flightPitch, 0, flightBank};
	pelvis->local.position.y += std::sin(time * 2.0f) * 0.015f * flightBlend;
	leftHip->local.rotation = glm::mix(leftHip->local.rotation, glm::vec3(10 + cruiseBlend * 8, 0, 5), flightBlend);
	rightHip->local.rotation = glm::mix(rightHip->local.rotation, glm::vec3(10 + cruiseBlend * 8, 0, -5), flightBlend);
	const glm::vec3 leftFlightArm(-25.0f - cruiseBlend * 100.0f, 0, 22.0f);
	const glm::vec3 rightFlightArm(-25.0f - cruiseBlend * 100.0f, 0, -22.0f);
	leftShoulder->local.rotation = glm::mix(leftShoulder->local.rotation, leftFlightArm, flightBlend);
	const glm::vec3 groundArm = rightShoulder->local.rotation;
	const glm::vec3 arm = glm::mix(glm::mix(groundArm, rightFlightArm, flightBlend),
		glm::vec3(-90 + laserPitch, 0, 0), rightArmOverride);
	// Cancel body lean/bank for the aiming wrist, preserving the beam's world direction.
	rightShoulder->local.rotation = glm::vec3(0);
	rightShoulder->local.basis = t3d::rotateZ(glm::radians(-flightBank * rightArmOverride))
		* t3d::rotateX(glm::radians(-flightPitch * rightArmOverride))
		* t3d::rotateX(glm::radians(arm.x)) * t3d::rotateZ(glm::radians(arm.z));
	head->local.rotation = glm::mix(head->local.rotation, glm::vec3(-flightPitch * 0.65f, 0, -flightBank * 0.5f), flightBlend);
}
