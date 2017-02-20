#include "geometry.h"

namespace pt {

void coordinate_system(const glm::vec3 &e1, glm::vec3 &e2, glm::vec3 &e3) {
	// Compute the tangent and bitangent
	if (std::abs(e1.x) > std::abs(e1.y)) {
		const float inv_len = 1.0 / std::sqrt(e1.x * e1.x + e1.y * e1.y);
		e2 = glm::vec3(-e1.z * inv_len, 0.f, e1.x * inv_len);
	} else {
		const float inv_len = 1.0 / std::sqrt(e1.y * e1.y + e1.z * e1.z);
		e2 = glm::vec3(0.f, e1.z * inv_len, -e1.y * inv_len);
	}
	e3 = glm::cross(e1, e2);
}

}

