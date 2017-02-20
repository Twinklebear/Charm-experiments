#pragma once

#include <glm/glm.hpp>

namespace pt {

struct DifferentialGeometry {
	glm::vec3 point, normal;
	glm::vec3 bitangent, tangent;
	// todo: pointer to the material

	// Make sure the shading space coordinate frame is orthonormal
	void orthonormalize();
	glm::vec3 to_shading(const glm::vec3 &v) const;
};

}

