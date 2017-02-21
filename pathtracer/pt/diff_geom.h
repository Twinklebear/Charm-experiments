#pragma once

#include <glm/glm.hpp>
#include "brdf.h"

namespace pt {

struct DifferentialGeometry {
	glm::vec3 point, normal;
	glm::vec3 bitangent, tangent;
	const BxDF *brdf = nullptr;

	// Make sure the shading space coordinate frame is orthonormal
	void orthonormalize();
	glm::vec3 to_shading(const glm::vec3 &v) const;
	glm::vec3 from_shading(const glm::vec3 &v) const;
};

}

