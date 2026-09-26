#include "Bullseye.h"

#include <cmath>

#include <glm/gtc/constants.hpp>

#include "render/Assets.h"
#include "scene/SceneNode.h"

Bullseye::Bullseye(SceneNode& parent, Assets& a, const glm::vec3& position, float headingDeg)
	: Character("Bullseye", parent.AddChild("Bullseye"))
{
	root->local.position = position;
	root->local.rotation.y = headingDeg;
	maxSpeed = 2.6f;
	turnRate = 100.0f;

	Material& coat = a.Mat("horse-coat", { 0.55f, 0.32f, 0.14f }, 0.25f, 20.0f);
	Material& mane = a.Mat("horse-mane", { 0.22f, 0.12f, 0.05f }, 0.2f, 16.0f);
	Material& muzzle = a.Mat("horse-muzzle", { 0.88f, 0.78f, 0.62f }, 0.2f, 16.0f);
	Material& hoof = a.Mat("horse-hoof", { 0.18f, 0.13f, 0.09f }, 0.5f, 32.0f);
	Material& saddleMat = a.Mat("saddle", { 0.55f, 0.12f, 0.08f }, 0.5f, 40.0f);
	Material& white = a.Mat("eye-white", { 1.0f, 1.0f, 1.0f }, 0.8f, 96.0f);
	Material& pupil = a.Mat("eye-pupil", { 0.05f, 0.05f, 0.08f }, 0.9f, 128.0f);
	Material& brass = a.Mat("brass", { 0.85f, 0.65f, 0.25f }, 0.8f, 64.0f);

	const Mesh& cube = a.Cube();
	const Mesh& sphere = a.Sphere();
	const Mesh& cylinder = a.Cylinder();
	const Mesh& cone = a.Cone();

	body = root->AddChild("Body");
	body->AddShape("Barrel", &sphere, &coat, { 0, 1.25f, 0 }, { 0.72f, 0.66f, 1.5f });
	body->AddShape("Belly", &sphere, &muzzle, { 0, 1.08f, 0 }, { 0.5f, 0.3f, 1.0f });

	// Saddle and the seat joint Jessie rides on
	SceneNode* saddle = body->AddChild("Saddle");
	saddle->local.position = { 0, 1.55f, 0.0f };
	saddle->AddShape("SaddlePad", &cube, &saddleMat, { 0, 0, 0 }, { 0.62f, 0.1f, 0.62f });
	saddle->AddShape("SaddleFlapL", &cube, &saddleMat, { 0.33f, -0.18f, 0 }, { 0.05f, 0.35f, 0.4f });
	saddle->AddShape("SaddleFlapR", &cube, &saddleMat, { -0.33f, -0.18f, 0 }, { 0.05f, 0.35f, 0.4f });
	saddle->AddShape("Horn", &cylinder, &brass, { 0, 0.1f, 0.26f }, { 0.07f, 0.16f, 0.07f });
	seat = saddle->AddChild("Seat");
	seat->local.position = { 0, 0.06f, -0.05f };

	// Neck tilted forward, head joint levelled back so the head points straight ahead
	neck = body->AddChild("Neck");
	neck->local.position = { 0, 1.42f, 0.62f };
	neck->local.rotation = { 35.0f, 0, 0 };
	neck->AddShape("NeckShape", &cylinder, &coat, { 0, 0.38f, 0 }, { 0.3f, 0.8f, 0.34f });
	neck->AddShape("Mane", &cube, &mane, { 0, 0.42f, -0.16f }, { 0.07f, 0.85f, 0.12f });

	head = neck->AddChild("Head");
	head->local.position = { 0, 0.8f, 0 };
	head->local.rotation = { -35.0f, 0, 0 };
	head->AddShape("Skull", &cube, &coat, { 0, 0.05f, 0.2f }, { 0.32f, 0.32f, 0.55f });
	head->AddShape("Muzzle", &cube, &muzzle, { 0, -0.02f, 0.5f }, { 0.28f, 0.26f, 0.26f });
	head->AddShape("NostrilL", &sphere, &hoof, { 0.07f, 0.0f, 0.63f }, glm::vec3(0.05f));
	head->AddShape("NostrilR", &sphere, &hoof, { -0.07f, 0.0f, 0.63f }, glm::vec3(0.05f));
	for (int side = -1; side <= 1; side += 2) {
		const float s = static_cast<float>(side);
		head->AddShape(side < 0 ? "EyeR" : "EyeL", &sphere, &white, { 0.165f * s, 0.12f, 0.25f }, { 0.04f, 0.1f, 0.1f });
		head->AddShape(side < 0 ? "PupilR" : "PupilL", &sphere, &pupil, { 0.18f * s, 0.12f, 0.27f }, { 0.03f, 0.06f, 0.06f });
		head->AddShape(side < 0 ? "EarR" : "EarL", &cone, &coat, { 0.1f * s, 0.3f, 0.02f }, { 0.1f, 0.22f, 0.08f });
	}
	head->AddShape("Forelock", &cube, &mane, { 0, 0.23f, 0.12f }, { 0.1f, 0.06f, 0.2f });

	// Tail
	tail = body->AddChild("Tail");
	tail->local.position = { 0, 1.42f, -0.72f };
	tail->local.rotation = { -30.0f, 0, 0 };
	tail->AddShape("TailShape", &cylinder, &mane, { 0, -0.35f, 0 }, { 0.1f, 0.7f, 0.1f });
	tail->AddShape("TailTip", &cone, &mane, { 0, -0.8f, 0 }, { 0.18f, 0.3f, 0.18f }, { 180.0f, 0, 0 });

	// Legs: hip joints at the body corners, legs hang along -Y
	const glm::vec3 hipPos[4] = { { 0.22f, 1.0f, 0.5f }, { -0.22f, 1.0f, 0.5f }, { 0.22f, 1.0f, -0.5f }, { -0.22f, 1.0f, -0.5f } };
	const char* hipName[4] = { "FrontLeftHip", "FrontRightHip", "BackLeftHip", "BackRightHip" };
	for (size_t i = 0; i < 4; ++i) {
		legs[i] = body->AddChild(hipName[i]);
		legs[i]->local.position = hipPos[i];
		legs[i]->AddShape("Leg", &cylinder, &coat, { 0, -0.45f, 0 }, { 0.15f, 0.9f, 0.15f });
		legs[i]->AddShape("Hoof", &cylinder, &hoof, { 0, -0.95f, 0 }, { 0.18f, 0.1f, 0.18f });
	}
	SaveHome();
}

void Bullseye::Animate(float dt, float time)
{
	Character::Animate(dt, time);

	// Gallop: diagonal pairs move together (FL with BR, FR with BL).
	const float amp = 32.0f * moveBlend;
	const float s = std::sin(walkPhase * 1.2f);
	legs[0]->local.rotation.x = s * amp;
	legs[3]->local.rotation.x = s * amp;
	legs[1]->local.rotation.x = -s * amp;
	legs[2]->local.rotation.x = -s * amp;

	body->local.position.y = std::abs(std::cos(walkPhase * 1.2f)) * 0.06f * moveBlend;
	neck->local.rotation.x = 35.0f + std::sin(walkPhase * 1.2f) * 6.0f * moveBlend;

	// Idle: tail swish and head look around; moving: tail streams back.
	const float idle = 1.0f - moveBlend;
	tail->local.rotation.z = std::sin(time * 2.5f) * 20.0f * idle;
	tail->local.rotation.x = -30.0f - 30.0f * moveBlend;
	head->local.rotation.y = std::sin(time * 0.5f) * 20.0f * idle;
}
