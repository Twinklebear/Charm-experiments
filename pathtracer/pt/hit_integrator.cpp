#include "hit_integrator.h"
#include "diff_geom.h"

namespace pt {

HitIntegrator::HitIntegrator(const std::vector<Sphere> &scene) : scene(scene) {}
glm::vec3 HitIntegrator::integrate(Ray &ray) const {
	for (const auto &s : scene) {
		DifferentialGeometry dg;
		if (s.intersect(ray, dg)) {
			return glm::vec3(1);
		}
	}
	return glm::vec3(0);
}

}

