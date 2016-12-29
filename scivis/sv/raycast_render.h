#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "bbox.h"
#include "ray.h"
#include "volume.h"

namespace sv {

class RaycastRender {
	float sampling_rate;
	std::shared_ptr<Volume> volume;

public:
	RaycastRender(float sampling_rate, std::shared_ptr<Volume> volume);
	glm::vec4 render(Ray &ray) const;
	glm::vec4 integrate_segment(const Ray &segment) const;
	float get_sampling_rate() const;
};

}

