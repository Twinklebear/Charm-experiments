#pragma once

#include "ray.h"
#include "diff_geom.h"

namespace pt {

class Geometry {
public:
	virtual ~Geometry(){}
	virtual bool intersect(Ray &ray, DifferentialGeometry &dg) const = 0;
};

// Compute an orthonormal coordinate system with e1 as one of the axes
void coordinate_system(const glm::vec3 &e1, glm::vec3 &e2, glm::vec3 &e3);

}

