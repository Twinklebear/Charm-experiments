#pragma once

#include <glm/glm.hpp>
#include "ray.h"
#include "diff_geom.h"

namespace pt {

class Sphere {
	glm::vec3 center;
	float radius;

public:
	Sphere(const glm::vec3 &center, const float radius);
	bool intersect(Ray &ray, DifferentialGeometry &dg) const;
};

}

