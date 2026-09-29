#pragma once

#include <glm/glm.hpp>

enum class CameraMode : int {
	Free = 0,   // fly anywhere: arrows move, right-drag looks around
	Orbit = 1,  // circle around the selected object for close inspection, scroll = distance
	Follow = 2, // third-person camera behind the selected object
};

// Perspective camera described by a position and yaw/pitch angles.
//   forward = (cos(pitch) sin(yaw), sin(pitch), -cos(pitch) cos(yaw))   (yaw 0 looks down -Z)
class Camera {
public:
	CameraMode mode = CameraMode::Free;

	glm::vec3 position{ 0.0f, 5.0f, 8.2f };
	float yaw = 0.0f;     // degrees
	float pitch = -12.0f; // degrees
	float fov = 65.0f;    // vertical field of view in degrees (zoom)
	float nearPlane = 0.05f;
	float farPlane = 200.0f;

	// Orbit / follow parameters
	glm::vec3 target{ 0.0f, 1.0f, 0.0f };
	float orbitDistance = 6.0f;
	float orbitYaw = 0.0f;
	float orbitPitch = 20.0f;

	glm::vec3 Forward() const;
	glm::vec3 Right() const;
	glm::vec3 Up() const;

	glm::mat4 View() const;
	glm::mat4 Projection(float aspect) const;

	// Places the camera on a sphere around `target` using orbitYaw / orbitPitch / orbitDistance
	// and turns it to look at the target.
	void ApplyOrbit();
	// Points yaw/pitch at a world position.
	void LookAt(const glm::vec3& point);
	void Reset();
};
