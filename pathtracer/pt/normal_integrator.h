#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "scene.h"
#include "ray.h"

namespace pt {

/* The normal integrator colors hit objects by their normal
 */
class NormalIntegrator {
	Scene scene;

public:
	NormalIntegrator(Scene scene);
	glm::vec3 integrate(Ray &ray) const;
};

}


