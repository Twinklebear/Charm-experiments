#include <glm/ext.hpp>
#include "ray.h"

namespace pt {

Ray::Ray(const glm::vec3 &origin, const glm::vec3 &dir, const float t_min, const float t_max)
	: origin(origin), dir(dir), t_min(t_min), t_max(t_max), depth(0)
{}
glm::vec3 Ray::at(const float t) const {
	return origin + dir * t;
}

}

std::ostream& operator<<(std::ostream &os, const pt::Ray &r) {
	os << "Ray {\n\torigin: " << glm::to_string(r.origin)
		<< "\n\tdir: " << glm::to_string(r.dir)
		<< "\n\tt_min: " << r.t_min
		<< "\n\tt_max: " << r.t_max
		<< "\n}";
	return os;
}

