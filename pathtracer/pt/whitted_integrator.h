#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "scene.h"
#include "ray.h"

namespace pt {

/* The Whitted integrator performs Whitted style
 * recursive ray tracing.
 */
class WhittedIntegrator {
	Scene scene;

public:
	WhittedIntegrator(Scene scene);
	glm::vec3 integrate(Ray &ray) const;
};

}

