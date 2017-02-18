#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "sphere.h"
#include "ray.h"

namespace pt {

/* The Whitted integrator performs Whitted style
 * recursive ray tracing.
 */
class WhittedIntegrator {
	std::vector<Sphere> scene;

public:
	WhittedIntegrator(const std::vector<Sphere> &scene);
	glm::vec3 integrate(Ray &ray) const;
};

}

