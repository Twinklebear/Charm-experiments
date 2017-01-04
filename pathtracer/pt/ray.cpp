#include "ray.h"

namespace sv {

Ray::Ray(const glm::vec3 &origin, const glm::vec3 &dir, const float t_min, const float t_max)
	: origin(origin), dir(dir), t_min(t_min), t_max(t_max)
{}
glm::vec3 Ray::at(const float t) const {
	return origin + dir * t;
}

}

