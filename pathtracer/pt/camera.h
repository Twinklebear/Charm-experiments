#pragma once

#include <array>
#include <glm/glm.hpp>
#include "ray.h"

namespace pt {

class Camera {
	size_t width, height;
	glm::vec3 pos, dir, up;
	glm::vec3 dir_top_left, screen_du, screen_dv;
	// Bounds of the image plane in the world
	glm::vec2 img_dims;
	// Unit vectors for the image plane axes
	glm::vec3 dx, dy;

public:
	Camera(const glm::vec3 &pos, const glm::vec3 &target, const glm::vec3 &up, const float fov_y,
			const size_t width, const size_t height);
	// Generate a ray going through the pixel at [x, y]. The coordinates should be in image space.
	Ray generate_ray(const float x, const float y, const std::array<float, 2> &samples) const;
	// Compute the pixel coordinates for a camera ray with some direction.
	// The direction passed should be normalized
	glm::vec2 project_ray(const glm::vec3 &r) const;
	// Get the eye position
	const glm::vec3& eye_pos() const;
};

}

