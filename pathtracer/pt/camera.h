#pragma once

#include <array>
#include <glm/glm.hpp>
#include "ray.h"

namespace sv {

class Camera {
	size_t width, height;
	glm::vec3 pos, dir, up;
	glm::vec3 dir_top_left, screen_du, screen_dv;

public:
	Camera(const glm::vec3 &pos, const glm::vec3 &target, const glm::vec3 &up, const float fov_y,
			const size_t width, const size_t height);
	// Generate a ray going through the pixel at [x, y]. The coordinates should be in image space.
	Ray generate_ray(const float x, const float y, const std::array<float, 2> &samples) const;
};

}

