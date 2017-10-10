#pragma once

#include <random>
#include <vector>
#include <glm/glm.hpp>
#include "scene.h"
#include "ray.h"
#include "integrator.h"

namespace pt {

/* The Whitted integrator performs Whitted style
 * recursive ray tracing.
 */
class PathIntegrator {
	std::mt19937 rng;
	std::uniform_real_distribution<float> brdf_sample;
	std::uniform_int_distribution<int> light_sample;

public:
	glm::vec3 background;
	std::shared_ptr<Scene> scene;

	PathIntegrator(const glm::vec3 &background, std::shared_ptr<Scene> scene);
	glm::vec3 integrate(Ray &ray);
	IntersectionResult integrate(const ActiveRay &ray);
	bool occluded(ActiveRay &ray);
};

}


