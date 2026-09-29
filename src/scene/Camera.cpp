#include "Camera.h"

#include <algorithm>
#include <cmath>

#include "math/Transform3D.h"

glm::vec3 Camera::Forward() const
{
	const float y = glm::radians(yaw), p = glm::radians(pitch);
	return glm::normalize(glm::vec3(std::cos(p) * std::sin(y), std::sin(p), -std::cos(p) * std::cos(y)));
}

glm::vec3 Camera::Right() const
{
	return glm::normalize(glm::cross(Forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 Camera::Up() const
{
	return glm::cross(Right(), Forward());
}

glm::mat4 Camera::View() const
{
	return t3d::lookAt(position, position + Forward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::Projection(float aspect) const
{
	return t3d::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void Camera::ApplyOrbit()
{
	orbitPitch = std::clamp(orbitPitch, -85.0f, 85.0f);
	orbitDistance = std::clamp(orbitDistance, 0.6f, 60.0f);
	const float y = glm::radians(orbitYaw), p = glm::radians(orbitPitch);
	const glm::vec3 offset(std::cos(p) * std::sin(y), std::sin(p), std::cos(p) * std::cos(y));
	position = target + offset * orbitDistance;
	LookAt(target);
}

void Camera::LookAt(const glm::vec3& point)
{
	if (glm::distance(point, position) < 1e-5f) return;
	const glm::vec3 d = glm::normalize(point - position);
	pitch = glm::degrees(std::asin(std::clamp(d.y, -1.0f, 1.0f)));
	yaw = glm::degrees(std::atan2(d.x, -d.z));
}

void Camera::Reset()
{
	mode = CameraMode::Free;
	position = { 0.0f, 5.0f, 8.2f };
	fov = 65.0f;
	LookAt({ 0.0f, 1.0f, 0.0f });
}
