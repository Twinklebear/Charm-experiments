#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "scene.h"
#include "ray.h"

namespace pt {

/* The hit integrator simply returns a white color if some object
 * was hit, and black if not
 */
class HitIntegrator {
	Scene scene;

public:
	HitIntegrator(Scene scene);
	glm::vec3 integrate(Ray &ray) const;
};

}

