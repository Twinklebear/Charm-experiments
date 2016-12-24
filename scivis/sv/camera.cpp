#include <cmath>
#include "camera.h"

namespace sv {

Camera::Camera(const glm::vec3 &pos, const glm::vec3 &target, const glm::vec3 &_up, const float fov_y,
		const size_t width, const size_t height)
	: width(width), height(height), pos(pos), dir(glm::normalize(target - pos)), up(glm::normalize(_up))
{
	const glm::vec3 dx = glm::normalize(glm::cross(dir, up));
	const glm::vec3 dy = glm::normalize(glm::cross(dx, dir));
	const float dim_y = 2.f * std::tan((fov_y / 2.f) * M_PI / 180.f);
	const float aspect_ratio = width / static_cast<float>(height);
	const float dim_x = dim_y * aspect_ratio;
	screen_du = dx * dim_x;
	screen_dv = dy * dim_y;
	dir_top_left = dir - 0.5f * screen_du - 0.5f * screen_dv;
}
Ray Camera::generate_ray(const float x, const float y, const std::array<float, 2> samples) const {
	glm::vec3 dir = dir_top_left;
	const glm::vec3 u_step = ((x + samples[0]) / width) * screen_du;
	const glm::vec3 v_step = ((y + samples[1]) / height) * screen_dv;
	dir = glm::normalize(dir + u_step + v_step);
	return Ray(pos, dir);
}

}

