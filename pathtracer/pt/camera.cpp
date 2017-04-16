#include <cmath>
#include <iostream>
#include <glm/ext.hpp>
#include "camera.h"

namespace pt {

Camera::Camera(const glm::vec3 &pos, const glm::vec3 &target, const glm::vec3 &_up, const float fov_y,
		const size_t width, const size_t height)
	: width(width), height(height), pos(pos), dir(glm::normalize(target - pos)), up(glm::normalize(_up))
{
	dx = glm::normalize(glm::cross(dir, up));
	dy = -glm::normalize(glm::cross(dx, dir));
	img_dims.y = 2.f * std::tan((fov_y / 2.f) * M_PI / 180.f);
	const float aspect_ratio = width / static_cast<float>(height);
	img_dims.x = img_dims.y * aspect_ratio;
	screen_du = dx * img_dims.x;
	screen_dv = dy * img_dims.y;
	dir_top_left = dir - 0.5f * screen_du - 0.5f * screen_dv;
}
Ray Camera::generate_ray(const float x, const float y, const std::array<float, 2> &samples) const {
	const glm::vec3 u_step = ((x + samples[0]) / width) * screen_du;
	const glm::vec3 v_step = ((y + samples[1]) / height) * screen_dv;
	const glm::vec3 dir = glm::normalize(dir_top_left + u_step + v_step);
	return Ray(pos, dir);
}
glm::vec2 Camera::project_ray(const glm::vec3 &r) const {
	// Intersect r with the plane, then compute difference between
	// dir_top_left and that vector, project onto dx, dy to recover the image
	// plane coordinates then scale into pixel coords.
	const float d = -glm::dot(pos + dir, dir);
	const float v = glm::dot(r, dir);
	if (std::abs(v) < 1e-6f){
		return glm::vec3(-1);
	}
	const float t = -(glm::dot(pos, dir) + d) / v;
	const glm::vec3 pt = pos + r * t;
	const glm::vec3 screen_diff = pt - pos - dir_top_left;
	return (glm::vec2(glm::dot(screen_diff, dx), glm::dot(screen_diff, dy)) / img_dims)
		* glm::vec2(width, height);
}
const glm::vec3& Camera::eye_pos() const {
	return pos;
}

}

