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
	glm::vec3 background;
	Scene scene;

public:
	WhittedIntegrator(const glm::vec3 &background, Scene scene);
	glm::vec3 integrate(Ray &ray) const;
};

}

