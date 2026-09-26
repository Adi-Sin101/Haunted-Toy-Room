#pragma once

#include <glm/glm.hpp>

#include "math/Transform3D.h"

// Local transformation of a scene node, kept as separate, human-editable parts:
//
//   M = T(position) * Ry(yaw) * Rx(pitch) * Rz(roll) * B * S(scale)
//
// Read right to left: scale the shape, apply the extra linear part B, roll, pitch, yaw, then move it.
// B ("basis") is an extra 3x3 linear transform kept as a matrix: the ball stores its accumulated
// rolling rotation there, and the inspector uses it for shear and reflection.
struct Transform {
	glm::vec3 position{ 0.0f };
	glm::vec3 rotation{ 0.0f }; // Euler angles in DEGREES: x = pitch, y = yaw (heading), z = roll
	glm::vec3 scale{ 1.0f };
	glm::mat4 basis{ 1.0f };

	glm::mat4 Matrix() const
	{
		return t3d::translate(position)
			* t3d::rotateY(glm::radians(rotation.y))
			* t3d::rotateX(glm::radians(rotation.x))
			* t3d::rotateZ(glm::radians(rotation.z))
			* basis
			* t3d::scale(scale);
	}
};
