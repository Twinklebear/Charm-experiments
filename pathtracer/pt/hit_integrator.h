#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "sphere.h"
#include "ray.h"

namespace pt {

/* The hit integrator simply returns a white color if some object
 * was hit, and black if not
 */
class HitIntegrator {
	std::vector<Sphere> scene;

public:
	HitIntegrator(const std::vector<Sphere> &scene);
	glm::vec3 integrate(Ray &ray) const;
};

}

