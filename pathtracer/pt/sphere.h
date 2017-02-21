#pragma once

#include <glm/glm.hpp>
#include "geometry.h"

namespace pt {

class Sphere : public Geometry {
	glm::vec3 center;
	float radius;

public:
	Sphere(const glm::vec3 &center, const float radius, std::shared_ptr<BxDF> &brdf);
	bool intersect(Ray &ray, DifferentialGeometry &dg) const override;
};

}

