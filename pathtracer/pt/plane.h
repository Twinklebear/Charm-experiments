#pragma once

#include <glm/glm.hpp>
#include "geometry.h"

namespace pt {

// Defines an infinite plane with some center position and normal
class Plane : public Geometry {
	glm::vec3 center, normal;
	float half_length;

public:
	Plane(const glm::vec3 &center, const glm::vec3 &normal, float half_length,
			std::shared_ptr<BxDF> &brdf);
	bool intersect(Ray &ray, DifferentialGeometry &dg) const override;
	BBox bounds() const override;
};

}


