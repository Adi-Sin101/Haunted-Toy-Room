#include "Cat.h"

#include <algorithm>
#include <cmath>

#include "render/Assets.h"
#include "scene/SceneNode.h"

Cat::Cat(SceneNode& parent, Assets& a, const glm::vec3& position, float headingDeg)
	: Character("Penny", parent.AddChild("Penny"))
{
	root->local.position = position;
	root->local.rotation.y = headingDeg;
	maxSpeed = 2.0f;
	turnRate = 160.0f;
	canFly = true;          // her route climbs the porch steps and the stairs (3D waypoints)
	clampToRoom = false;    // she starts in the garden, outside the room

	// Colours sampled from the reference photos: creamy white fur, ginger patches, pink nose and ears.
	Material& fur = a.Mat("penny-fur", {0.97f, 0.94f, 0.88f}, 0.06f, 8.0f);
	Material& ginger = a.Mat("penny-ginger", {0.90f, 0.56f, 0.24f}, 0.06f, 8.0f);
	Material& pink = a.Mat("penny-pink", {0.93f, 0.62f, 0.62f}, 0.2f, 16.0f);
	Material& earPink = a.Mat("penny-ear-pink", {0.95f, 0.76f, 0.74f}, 0.05f, 8.0f);
	Material& iris = a.Mat("penny-eyes", {0.66f, 0.70f, 0.32f}, 0.8f, 96.0f);
	Material& pupil = a.Mat("eye-pupil", {0.05f, 0.05f, 0.08f}, 0.9f, 128.0f);
	Material& lash = a.Mat("penny-closed-eye", {0.22f, 0.16f, 0.12f}, 0.0f);
	Material& whisker = a.Mat("penny-whisker", {0.92f, 0.92f, 0.92f}, 0.0f);
	fur.texture = a.SlotTexture(Assets::FabricSlot); fur.rtTextureSlot = Assets::FabricSlot; fur.uvScale = {4, 4};
	ginger.texture = fur.texture; ginger.rtTextureSlot = Assets::FabricSlot; ginger.uvScale = {4, 4};

	const Mesh& sphere = a.Sphere();
	const Mesh& cylinder = a.Cylinder();
	const Mesh& cone = a.Cone();
	const Mesh& cube = a.Cube();

	// Body: an ellipsoid torso with a rounder chest and haunch; two ginger patches drape over the left
	// flank (+X) and back - the big mid-back patch and the one over the hip.
	body = root->AddChild("Body");
	body->local.position = {0, ShoulderHeight, 0};
	body->AddShape("Torso", &sphere, &fur, {0, 0, -0.02f}, {0.64f, 0.58f, 1.30f});
	body->AddShape("Chest", &sphere, &fur, {0, 0.03f, 0.42f}, {0.58f, 0.60f, 0.62f});
	body->AddShape("Haunch", &sphere, &fur, {0, 0, -0.42f}, {0.62f, 0.62f, 0.66f});
	body->AddShape("BackPatch", &sphere, &ginger, {0.12f, 0.20f, -0.02f}, {0.46f, 0.24f, 0.52f}, {0, 0, -22});
	body->AddShape("HipPatch", &sphere, &ginger, {0.15f, 0.18f, -0.46f}, {0.48f, 0.26f, 0.50f}, {0, 0, -26});

	// Head joint at the top of the chest.
	head = body->AddChild("Head");
	head->local.position = {0, 0.26f, 0.74f};
	head->AddShape("Skull", &sphere, &fur, {0, 0, 0}, {0.46f, 0.40f, 0.42f});
	head->AddShape("Muzzle", &sphere, &fur, {0, -0.09f, 0.17f}, {0.30f, 0.18f, 0.18f});
	head->AddShape("Chin", &sphere, &fur, {0, -0.15f, 0.14f}, {0.14f, 0.08f, 0.10f});
	// Ginger cap over the top of the head and down around her left eye (the right side stays white)
	// (an almost skull-sized sphere shifted up and to her left: only its top-left part rises above the
	// skull surface, so it reads as a patch of fur rather than a separate shape)
	head->AddShape("GingerCap", &sphere, &ginger, {0.05f, 0.07f, -0.02f}, {0.40f, 0.30f, 0.38f});
	head->AddShape("GingerEyePatch", &sphere, &ginger, {0.11f, 0.05f, 0.13f}, {0.20f, 0.17f, 0.12f});
	for (float side : {1.0f, -1.0f}) {
		const Material& earColor = side > 0 ? ginger : fur;
		head->AddShape("Ear", &cone, &earColor, {side * 0.13f, 0.21f, -0.03f}, {0.20f, 0.22f, 0.11f}, {-10, 0, -side * 16});
		head->AddShape("InnerEar", &cone, &earPink, {side * 0.13f, 0.20f, 0.012f}, {0.13f, 0.16f, 0.03f}, {-10, 0, -side * 16});
		const size_t e = side > 0 ? 0 : 1;
		openEyes[e] = head->AddShape("Eye", &sphere, &iris, {side * 0.095f, 0.03f, 0.19f}, {0.075f, 0.05f, 0.03f});
		openEyes[e + 2] = head->AddShape("Pupil", &sphere, &pupil, {side * 0.095f, 0.03f, 0.203f}, {0.014f, 0.045f, 0.012f});
		closedEyes[e] = head->AddShape("ClosedEye", &cube, &lash, {side * 0.095f, 0.025f, 0.2f}, {0.07f, 0.008f, 0.012f}, {0, 0, side * 8});
		closedEyes[e]->visible = false;
		for (float tilt : {-6.0f, 8.0f})
			head->AddShape("Whisker", &cylinder, &whisker, {side * 0.17f, -0.08f + tilt * 0.002f, 0.21f}, {0.008f, 0.34f, 0.008f}, {0, side * 12, side * (80 + tilt)});
	}
	head->AddShape("Nose", &sphere, &pink, {0, -0.03f, 0.27f}, {0.06f, 0.04f, 0.04f});

	// Tail: ginger at the base (continuing the hip patch), white towards the tip.
	tail = body->AddChild("Tail");
	tail->local.position = {0, 0.12f, -0.74f};
	tail->AddShape("TailBase", &cylinder, &ginger, {0, 0.22f, 0}, {0.11f, 0.46f, 0.11f});
	tailTip = tail->AddChild("TailTip");
	tailTip->local.position = {0, 0.44f, 0};
	tailTip->AddShape("TailEnd", &cylinder, &fur, {0, 0.2f, 0}, {0.10f, 0.42f, 0.10f});
	tailTip->AddShape("TailTipFur", &sphere, &fur, {0, 0.41f, 0}, glm::vec3(0.11f));

	// Legs: front pair under the chest, back pair under the haunch; white legs and paws.
	const glm::vec3 hips[4] = {{0.17f, -0.08f, 0.42f}, {-0.17f, -0.08f, 0.42f}, {0.18f, -0.06f, -0.44f}, {-0.18f, -0.06f, -0.44f}};
	for (size_t i = 0; i < 4; ++i) {
		legs[i] = body->AddChild(i < 2 ? "FrontLeg" : "BackLeg");
		legs[i]->local.position = hips[i];
		const float length = i < 2 ? 0.46f : 0.48f;
		legs[i]->AddShape("Leg", &cylinder, &fur, {0, -length * 0.5f, 0}, {0.15f, length, 0.15f});
		legs[i]->AddShape("Paw", &sphere, &fur, {0, -length - 0.02f, 0.04f}, {0.17f, 0.09f, 0.22f});
	}
	SaveHome();
}

glm::vec3 Cat::HeadPosition() const
{
	return head->WorldPosition();
}

void Cat::Animate(float dt, float time)
{
	Character::Animate(dt, time);
	const float k = 1.0f - std::exp(-dt * 5.0f);
	sitBlend += ((pose == Pose::Sit ? 1.0f : 0.0f) - sitBlend) * k;
	sleepBlend += ((pose == Pose::Sleep ? 1.0f : 0.0f) - sleepBlend) * k * 0.6f;
	slopeBlend += (slope - slopeBlend) * k;
	const float rest = std::max(sitBlend, sleepBlend);

	// Walk: diagonal pairs (front-left with back-right) swing together; the body bobs twice a stride.
	const float stride = std::sin(walkPhase * 1.6f) * 30.0f * moveBlend * (1.0f - rest);
	float bob = std::abs(std::cos(walkPhase * 1.6f)) * 0.03f * moveBlend;

	// Body pitch: stairs (nose up), sitting (front raised); roll onto her side when asleep.
	const float pitch = -slopeBlend * (1.0f - rest) - 30.0f * sitBlend * (1.0f - sleepBlend);
	body->local.rotation = {pitch, 0, 88.0f * sleepBlend};
	body->local.position = {0, glm::mix(glm::mix(ShoulderHeight, 0.47f, sitBlend), 0.31f, sleepBlend) + bob, -0.12f * sitBlend * (1.0f - sleepBlend)};

	// Legs: gait, then the seated fold (front legs vertical, hind legs tucked forward), then stretched out asleep.
	const float frontSit = 30.0f, backSit = -62.0f;
	const float gait[4] = {stride, -stride, -stride, stride};
	for (size_t i = 0; i < 4; ++i) {
		float angle = gait[i] + slopeBlend * (1.0f - rest);           // keep the legs vertical on the stairs
		angle = glm::mix(angle, i < 2 ? frontSit : backSit, sitBlend * (1.0f - sleepBlend));
		angle = glm::mix(angle, i < 2 ? -35.0f : 25.0f, sleepBlend);
		legs[i]->local.rotation = {angle, 0, 0};
	}

	// Tail: raised and curving while she walks, curled around her when she sits, flat when asleep.
	tail->local.rotation = {glm::mix(glm::mix(-35.0f, -95.0f, sitBlend), -100.0f, sleepBlend), 0,
		std::sin(time * 2.2f) * 12.0f * (1.0f - sleepBlend)};
	tailTip->local.rotation = {glm::mix(-25.0f, 15.0f, rest), 0, glm::mix(0.0f, 40.0f * sitBlend, 1.0f - sleepBlend)};

	// Head: follows the look target (yaw relative to her heading), levelled against the body pitch.
	float targetYaw = 0.0f;
	if (looking) {
		const glm::vec3 d = lookTarget - head->WorldPosition();
		targetYaw = std::clamp(WrapDegrees(glm::degrees(std::atan2(d.x, d.z)) - Heading()), -70.0f, 70.0f);
	}
	headYaw += (targetYaw - headYaw) * k;
	head->local.rotation = {-pitch * 0.8f + 18.0f * sleepBlend, headYaw * (1.0f - sleepBlend), -20.0f * sleepBlend};

	// Eyes: closed while asleep, plus a short blink every few seconds while awake.
	const bool blink = std::fmod(time + 1.3f, 4.1f) < 0.13f;
	const bool closed = sleepBlend > 0.5f || blink;
	for (SceneNode* e : openEyes) e->visible = !closed;
	for (SceneNode* e : closedEyes) e->visible = closed;
}
