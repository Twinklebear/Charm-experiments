#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "scene.h"
#include "ray.h"
#include "integrator.h"

namespace pt {

/* The Whitted integrator performs Whitted style
 * recursive ray tracing.
 */
struct WhittedIntegrator {
	glm::vec3 background;
	Scene scene;

	WhittedIntegrator(const glm::vec3 &background, Scene scene);
	IntersectionResult integrate(ActiveRay &ray) const;
	bool occluded(ActiveRay &ray) const;
};

}

