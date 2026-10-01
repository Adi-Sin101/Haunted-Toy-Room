#pragma once

#include <cmath>

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

	// Same product as above, multiplied out by hand: it runs for every node every frame, and five
	// general 4x4 products per node were a large part of the frame time in Debug builds.
	//   Ry * Rx * Rz = | cy*cz + sy*sx*sz   -cy*sz + sy*sx*cz   sy*cx |
	//                  | cx*sz               cx*cz              -sx    |
	//                  | -sy*cz + cy*sx*sz   sy*sz + cy*sx*cz   cy*cx |
	glm::mat4 Matrix() const
	{
		const glm::vec3 r = glm::radians(rotation);
		const float cx = std::cos(r.x), sx = std::sin(r.x);
		const float cy = std::cos(r.y), sy = std::sin(r.y);
		const float cz = std::cos(r.z), sz = std::sin(r.z);
		glm::mat4 m(1.0f); // glm stores columns: m[column][row]
		m[0] = glm::vec4(cy * cz + sy * sx * sz, cx * sz, -sy * cz + cy * sx * sz, 0.0f);
		m[1] = glm::vec4(-cy * sz + sy * sx * cz, cx * cz, sy * sz + cy * sx * cz, 0.0f);
		m[2] = glm::vec4(sy * cx, -sx, cy * cx, 0.0f);
		if (basis != glm::mat4(1.0f))
			m = m * basis;
		m[0] *= scale.x;
		m[1] *= scale.y;
		m[2] *= scale.z;
		m[3] += glm::vec4(position, 0.0f); // T * M only adds the translation to the last column
		return m;
	}
};
