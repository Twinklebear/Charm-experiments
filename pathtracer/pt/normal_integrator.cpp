#include "normal_integrator.h"
#include "diff_geom.h"

namespace pt {

NormalIntegrator::NormalIntegrator(Scene scene) : scene(std::move(scene)) {}
glm::vec3 NormalIntegrator::integrate(Ray &ray) const {
	DifferentialGeometry dg;
	if (scene.intersect(ray, dg)) {
		dg.orthonormalize();
		return (glm::vec3(1) + dg.normal) * 0.5f;
	}
	return glm::vec3(0);
}

}


