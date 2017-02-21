#include "diff_geom.h"

namespace pt {

void DifferentialGeometry::orthonormalize() {
	normal = glm::normalize(normal);
	bitangent = glm::normalize(bitangent);
	tangent = glm::cross(normal, bitangent);
	bitangent = glm::cross(tangent, normal);
}
glm::vec3 DifferentialGeometry::to_shading(const glm::vec3 &v) const {
	return glm::vec3(glm::dot(v, bitangent), glm::dot(v, tangent), glm::dot(v, normal));
}
glm::vec3 DifferentialGeometry::from_shading(const glm::vec3 &v) const {
	return glm::vec3(bitangent.x * v.x + tangent.x * v.y + normal.x * v.z,
			bitangent.y * v.x + tangent.y * v.y + normal.y * v.z,
			bitangent.z * v.x + tangent.z * v.y + normal.z * v.z);
}

}

