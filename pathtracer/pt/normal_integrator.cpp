#include "normal_integrator.h"
#include "diff_geom.h"

namespace pt {

NormalIntegrator::NormalIntegrator(const std::vector<Sphere> &scene) : scene(scene) {}
glm::vec3 NormalIntegrator::integrate(Ray &ray) const {
	DifferentialGeometry dg;
	bool hit = false;
	for (const auto &s : scene) {
		hit = s.intersect(ray, dg) || hit;
	}
	if (hit) {
		return (glm::vec3(1) + dg.normal) * 0.5f;
	}
	return glm::vec3(0);
}

}


