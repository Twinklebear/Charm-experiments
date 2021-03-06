#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
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
		return integrate_segment(ray);
	} else {
		return glm::vec4(0.0);
	}
}
glm::vec4 RaycastRender::integrate_segment(const Ray &segment) const {
	const float dt = 1.0 / sampling_rate;
	const glm::vec3 vol_offset(volume->get_offset());
	glm::vec4 color = glm::vec4(0.0);
	glm::vec3 pos = segment.at(segment.t_min);
	for (float t = segment.t_min; t < segment.t_max; t += dt) {
		const float value = (volume->sample(pos - vol_offset) - volume->get_min())
			/ (volume->get_max() - volume->get_min());
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
float RaycastRender::get_sampling_rate() const {
	return sampling_rate;
}

}

