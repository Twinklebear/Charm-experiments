#pragma once

#include <limits>
#include <glm/glm.hpp>

namespace sv {

struct Ray {
	glm::vec3 origin, dir;
	float t_min, t_max;

	Ray(const glm::vec3 &origin, const glm::vec3 &dir, const float t_min = 0.f,
			const float t_max = std::numeric_limits<float>::infinity());
	glm::vec3 at(const float t) const;
};

}

