#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include "raycast_render.h"

namespace sv {

RaycastRender::RaycastRender(float sampling_rate, std::shared_ptr<Volume> volume)
	: sampling_rate(sampling_rate), volume(volume)
{}
glm::vec4 RaycastRender::render(Ray &ray) const {
	const glm::vec3 inv_dir = glm::vec3(1.f) / ray.dir;
	const std::array<int, 3> neg_dir = {
		ray.dir.x < 0 ? 1 : 0, ray.dir.y < 0 ? 1 : 0, ray.dir.z < 0 ? 1 : 0
	};
	if (volume->get_bounds().intersect(ray, inv_dir, neg_dir, &ray.t_min, &ray.t_max)) {
		std::cout << "box hit, t_min = " << ray.t_min << ", t_max = "
			<< ray.t_max << "\n";
		return integrate_segment(*volume, ray);
	} else {
		return glm::vec4(1.0, 0.0, 0.0, 1.0);
	}
}
glm::vec4 RaycastRender::integrate_segment(const Volume &vol, const Ray &segment) const {
	const glm::uvec3 vol_dims = vol.get_dims();
	const float dt = std::min(vol_dims.x, std::min(vol_dims.y, vol_dims.z)) * sampling_rate;

	glm::vec4 color = glm::vec4(0.0);
	glm::vec3 pos = segment.at(segment.t_min);
	for (float t = segment.t_min; t < segment.t_max; t += dt) {
		const float value = (vol.sample(pos) - vol.get_min()) / (vol.get_max() - vol.get_min());
		// TODO: Actual transfer function, this is just a grayscale ramp
		glm::vec4 tfn_sample = glm::vec4(value);
		glm::vec3 col = (1.f - color.w) * tfn_sample.w * glm::vec3(tfn_sample);
		color.x += col.x;
		color.y += col.y;
		color.z += col.z;
		color.w += (1.0 - color.w) * tfn_sample.w;
		if (color.w >= 0.98) {
			break;
		}
		pos += dt * segment.dir;
	}
	return color;
}

}

