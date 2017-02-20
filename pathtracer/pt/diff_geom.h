#pragma once

#include <glm/glm.hpp>

namespace pt {

struct DifferentialGeometry {
	glm::vec3 point, normal;
	glm::vec3 tangent, bitangent;
	// pointer to the material
};

}

