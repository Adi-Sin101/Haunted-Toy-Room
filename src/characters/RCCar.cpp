#include "RCCar.h"

#include <algorithm>
#include <cmath>

#include "render/Assets.h"
#include "scene/SceneNode.h"

RCCar::RCCar(SceneNode& parent, Assets& a, const glm::vec3& position, float headingDeg)
	: Character("RC Car", parent.AddChild("RCCar"))
{
	root->local.position = position;
	root->local.rotation.y = headingDeg;
	maxSpeed = 3.5f;
	acceleration = 6.0f;
	turnRate = 110.0f;

	Material& paint = a.Mat("car-paint", { 0.85f, 0.1f, 0.1f }, 0.9f, 96.0f);
	paint.reflectivity = 0.15f;
	Material& trim = a.Mat("car-trim", { 0.12f, 0.12f, 0.14f }, 0.4f, 32.0f);
	Material& glass = a.Mat("car-glass", { 0.25f, 0.45f, 0.7f }, 1.0f, 128.0f);
	glass.reflectivity = 0.3f;
	Material& tyre = a.Mat("tyre", { 0.08f, 0.08f, 0.08f }, 0.1f, 8.0f);
	tyre.texture = a.SlotTexture(Assets::DenimSlot); tyre.rtTextureSlot = Assets::DenimSlot; tyre.uvScale = {6, 1}; // rubber tread grain
	Material& hub = a.Mat("hub", { 0.75f, 0.75f, 0.8f }, 0.9f, 96.0f);
	Material& lamp = a.Mat("car-headlight", { 1.0f, 1.0f, 0.85f }, 0.5f, 32.0f);
	lampMaterial = &lamp;

	const Mesh& cube = a.Cube();
	const Mesh& sphere = a.Sphere();
	const Mesh& cylinder = a.Cylinder();

	SceneNode* chassis = root->AddChild("Chassis");
	chassis->AddShape("Body", &cube, &paint, { 0, 0.38f, 0 }, { 0.8f, 0.3f, 1.4f });
	chassis->AddShape("Cabin", &cube, &paint, { 0, 0.66f, -0.15f }, { 0.7f, 0.28f, 0.7f });
	chassis->AddShape("Windshield", &cube, &glass, { 0, 0.66f, 0.205f }, { 0.62f, 0.22f, 0.02f });
	chassis->AddShape("RearWindow", &cube, &glass, { 0, 0.66f, -0.505f }, { 0.62f, 0.22f, 0.02f });
	chassis->AddShape("SideWindowL", &cube, &glass, { 0.355f, 0.66f, -0.15f }, { 0.02f, 0.2f, 0.58f });
	chassis->AddShape("SideWindowR", &cube, &glass, { -0.355f, 0.66f, -0.15f }, { 0.02f, 0.2f, 0.58f });
	chassis->AddShape("BumperF", &cube, &trim, { 0, 0.28f, 0.72f }, { 0.84f, 0.1f, 0.06f });
	chassis->AddShape("BumperB", &cube, &trim, { 0, 0.28f, -0.72f }, { 0.84f, 0.1f, 0.06f });
	chassis->AddShape("SpoilerPostL", &cube, &trim, { 0.3f, 0.6f, -0.62f }, { 0.05f, 0.18f, 0.05f });
	chassis->AddShape("SpoilerPostR", &cube, &trim, { -0.3f, 0.6f, -0.62f }, { 0.05f, 0.18f, 0.05f });
	chassis->AddShape("Spoiler", &cube, &trim, { 0, 0.7f, -0.64f }, { 0.9f, 0.04f, 0.2f });
	chassis->AddShape("Antenna", &cylinder, &trim, { 0.25f, 0.95f, -0.4f }, { 0.02f, 0.5f, 0.02f });
	chassis->AddShape("AntennaTip", &sphere, &paint, {0.25f, 1.20f, -0.4f}, glm::vec3(0.055f));
	chassis->AddShape("Grille", &cube, &trim, {0, 0.39f, 0.706f}, {0.38f, 0.1f, 0.016f});

	for (int i = 0; i < 2; ++i) {
		const float x = i == 0 ? 0.25f : -0.25f;
		chassis->AddShape(i == 0 ? "HeadlightBulbL" : "HeadlightBulbR", &sphere, &lamp, { x, 0.42f, 0.7f }, glm::vec3(0.12f));
		headlights[static_cast<size_t>(i)] = chassis->AddChild(i == 0 ? "HeadlightL" : "HeadlightR");
		headlights[static_cast<size_t>(i)]->local.position = { x, 0.42f, 0.78f };
	}

	const glm::vec3 wheelPos[4] = { { 0.45f, WheelRadius, 0.45f }, { -0.45f, WheelRadius, 0.45f },
	                                { 0.45f, WheelRadius, -0.45f }, { -0.45f, WheelRadius, -0.45f } };
	const char* names[4] = { "WheelFL", "WheelFR", "WheelBL", "WheelBR" };
	for (size_t i = 0; i < 4; ++i) {
		steerJoints[i] = root->AddChild(names[i]);
		steerJoints[i]->local.position = wheelPos[i];
		spinJoints[i] = steerJoints[i]->AddChild("Spin");
		// Cylinder axis is Y; roll it 90 deg about Z so the axle points along X.
		spinJoints[i]->AddShape("Tyre", &cylinder, &tyre, { 0, 0, 0 }, { 2 * WheelRadius, 0.2f, 2 * WheelRadius }, { 0, 0, 90 });
		spinJoints[i]->AddShape("Hub", &cylinder, &hub, { 0, 0, 0 }, { 0.2f, 0.22f, 0.2f }, { 0, 0, 90 });
		spinJoints[i]->AddShape("Spoke", &cube, &hub, { 0, 0, 0 }, { 0.23f, 0.05f, 0.36f });
		spinJoints[i]->AddShape("RadialSpoke", &cube, &hub, {0, 0, 0}, {0.23f, 0.05f, 0.36f}, {90, 0, 0});
	}
	SaveHome();
}

float RCCar::TurnFactor() const
{
	// A car turns only while rolling; reversing flips the steering direction.
	return std::clamp(speed / maxSpeed, -1.0f, 1.0f) * 1.3f;
}

void RCCar::Animate(float dt, float time)
{
	Character::Animate(dt, time);

	wheelAngle += glm::degrees(speed * dt / WheelRadius);
	steering += (steerInput * 28.0f - steering) * std::min(1.0f, dt * 10.0f);
	for (size_t i = 0; i < 4; ++i) {
		spinJoints[i]->local.rotation.x = wheelAngle;
		steerJoints[i]->local.rotation.y = i < 2 ? steering : 0.0f;
	}
	lampMaterial->emissive = headlightsOn ? glm::vec3(1.0f, 0.95f, 0.7f) : glm::vec3(0.0f);
	steerInput = 0.0f;
}
