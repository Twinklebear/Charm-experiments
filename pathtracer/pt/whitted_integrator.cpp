#include "whitted_integrator.h"
#include "diff_geom.h"
#include "light.h"
#include "brdf.h"

namespace pt {

WhittedIntegrator::WhittedIntegrator(Scene scene) : scene(std::move(scene)) {}
glm::vec3 WhittedIntegrator::integrate(Ray &ray) const {
	DifferentialGeometry dg;
	if (scene.intersect(ray, dg)) {
		dg.orthonormalize();
		const DirectionalLight dir_light(glm::vec3(-1), glm::vec3(0.8));
		const PointLight pt_light(glm::vec3(1, 2, 2), glm::vec3(2.0));
		const Lambertian material(glm::vec3(0.1, 0.1, 0.8));

		const LightSample light_sample = pt_light.incident(dg.point);
		glm::vec3 lighting(0);
		// TODO: Should detect transmission vs. reflection case and request appropriate brdf
		if (glm::dot(light_sample.dir, dg.normal) > 0.0 && !light_sample.occluded(scene)) {
			const glm::vec3 w_i = dg.to_shading(light_sample.dir);
			const glm::vec3 w_o = dg.to_shading(-ray.dir);
			// note: no division by pdf since it's 1 for the delta light
			lighting = material.eval(w_i, w_o) * light_sample.illum
				* std::abs(glm::dot(light_sample.dir, dg.normal));
		}
		return lighting;
	}
	return glm::vec3(0);
}

}


