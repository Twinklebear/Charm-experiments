#pragma once

#include <random>
#include <vector>
#include <glm/glm.hpp>
#include "scene.h"
#include "ray.h"

namespace pt {

/* The Whitted integrator performs Whitted style
 * recursive ray tracing.
 */
class PathIntegrator {
	glm::vec3 background;
	Scene scene;
	mutable std::mt19937 rng;
	mutable std::uniform_real_distribution<float> brdf_sample;
	mutable std::uniform_int_distribution<int> light_sample;

public:
	PathIntegrator(const glm::vec3 &background, Scene scene);
	glm::vec3 integrate(Ray &ray) const;
};

}


