#include "hit_integrator.h"
#include "diff_geom.h"

namespace pt {

HitIntegrator::HitIntegrator(Scene scene) : scene(std::move(scene)) {}
glm::vec3 HitIntegrator::integrate(Ray &ray) const {
	DifferentialGeometry dg;
	if (scene.intersect(ray, dg)) {
		return glm::vec3(1);
	}
	return glm::vec3(0);
}

}

