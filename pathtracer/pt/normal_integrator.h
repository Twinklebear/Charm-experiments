#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "sphere.h"
#include "ray.h"

namespace pt {

/* The normal integrator colors hit objects by their normal
 */
class NormalIntegrator {
	std::vector<Sphere> scene;

public:
	NormalIntegrator(const std::vector<Sphere> &scene);
	glm::vec3 integrate(Ray &ray) const;
};

}


