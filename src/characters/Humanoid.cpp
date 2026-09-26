#include "Humanoid.h"

#include <algorithm>
#include <cmath>

#include "render/Assets.h"
#include "scene/SceneNode.h"

namespace {

// Builds one leg under a hip joint: the joint sits at the hip, the leg hangs down its local -Y,
// so rotating the joint about X swings the whole leg (leg + boot + foot) like a pendulum.
void BuildLeg(SceneNode* hip, Assets& a, const Material& pants, const Material& boots)
{
	hip->AddShape("Leg", &a.Cylinder(), &pants, { 0, -0.31f, 0 }, { 0.15f, 0.62f, 0.15f });
	hip->AddShape("BootShaft", &a.Cylinder(), &boots, { 0, -0.67f, 0 }, { 0.18f, 0.22f, 0.18f });
	hip->AddShape("Foot", &a.Cube(), &boots, { 0, -0.80f, 0.05f }, { 0.18f, 0.10f, 0.30f });
}

SceneNode* BuildArm(SceneNode* shoulder, Assets& a, const Material& sleeve, const Material& hand)
{
	shoulder->AddShape("UpperArm", &a.Cylinder(), &sleeve, { 0, -0.25f, 0 }, { 0.11f, 0.5f, 0.11f });
	shoulder->AddShape("ShoulderBall", &a.Sphere(), &sleeve, { 0, 0, 0 }, glm::vec3(0.13f));
	return shoulder->AddShape("Hand", &a.Sphere(), &hand, { 0, -0.55f, 0 }, glm::vec3(0.13f));
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

	if (!st.spaceRanger) {
		head->AddShape("Hair", &sphere, &hair, { 0, 0.27f, -0.035f }, { 0.36f, 0.32f, 0.35f });
		if (st.longHair) {
			SceneNode* braid = head->AddChild("Braid");
			braid->local.position = { 0, 0.22f, -0.17f };
			braid->local.rotation = { -12.0f, 0, 0 };
			braid->AddShape("Plait", &cylinder, &hair, { 0, -0.28f, 0 }, { 0.1f, 0.56f, 0.1f });
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
	const float swing = std::sin(walkPhase) * 35.0f * moveBlend * (1.0f - seatBlend);

	leftHip->local.rotation = glm::mix(glm::vec3(swing, 0, 0), glm::vec3(-40, 0, 32), seatBlend);
	rightHip->local.rotation = glm::mix(glm::vec3(-swing, 0, 0), glm::vec3(-40, 0, -32), seatBlend);

	const glm::vec3 leftArm = glm::mix(glm::vec3(-swing * 0.8f, 0, 6), glm::vec3(-50, 0, 8), seatBlend);
	glm::vec3 rightArm = glm::mix(glm::vec3(swing * 0.8f, 0, -6), glm::vec3(-50, 0, -8), seatBlend);
	rightArm = glm::mix(rightArm, glm::vec3(-90, 0, 0), rightArmOverride);
	leftShoulder->local.rotation = leftArm;
	rightShoulder->local.rotation = rightArm;

	// Body bob while walking, idle "looking around" when standing still.
	torso->Parent()->local.position.y = HipHeight + std::abs(std::sin(walkPhase)) * 0.04f * moveBlend;
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

	// Wings: a joint on the back whose X scale opens / folds both wings
	wings = torso->AddChild("Wings");
	wings->local.position = { 0, 0.33f, -0.16f };
	wings->AddShape("Pack", &cube, &white, { 0, 0, 0 }, { 0.3f, 0.3f, 0.1f });
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
}

glm::vec3 Buzz::LaserTip() const
{
	return laserTip->WorldPosition();
}

void Buzz::Animate(float dt, float time)
{
	const float k = std::min(1.0f, dt * 6.0f);
	rightArmOverride += ((laserOn ? 1.0f : 0.0f) - rightArmOverride) * k;
	Humanoid::Animate(dt, time);

	laserBeam->visible = laserOn && rightArmOverride > 0.9f;

	// Wings open while airborne; legs trail back a little in flight.
	const bool flying = root->local.position.y > 0.05f;
	wingOpen += ((flying ? 1.0f : 0.0f) - wingOpen) * k;
	wings->local.scale = { 0.08f + 0.92f * wingOpen, 1.0f, 1.0f };
	if (flying) {
		leftHip->local.rotation.x += 15.0f * wingOpen;
		rightHip->local.rotation.x += 15.0f * wingOpen;
	}
}
