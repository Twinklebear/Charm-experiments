#include "whitted_integrator.h"
#include "diff_geom.h"
#include "light.h"
#include "brdf.h"

namespace pt {

WhittedIntegrator::WhittedIntegrator(Scene scene) : scene(std::move(scene)) {}
glm::vec3 WhittedIntegrator::integrate(Ray &ray) const {
	const float dummy_samples[2] = {0};
	DifferentialGeometry dg;
	if (scene.intersect(ray, dg)) {
		dg.orthonormalize();
		const DirectionalLight dir_light(glm::vec3(-1), glm::vec3(0.8));
		const PointLight pt_light(glm::vec3(0, 0, 1.4), glm::vec3(2.0));

		const LightSample light_sample = pt_light.incident(dg.point);
		glm::vec3 lighting(0);
		const glm::vec3 w_i = dg.to_shading(light_sample.dir);
		const glm::vec3 w_o = dg.to_shading(-ray.dir);
		// TODO: Should detect transmission vs. reflection case and request appropriate brdf
		if (!(dg.brdf->bxdf_type() & BRDFType::Specular)) {
			if (glm::dot(light_sample.dir, dg.normal) > 0.0 && !light_sample.occluded(scene)) {
				// note: no division by pdf since it's 1 for the delta light
				lighting = dg.brdf->eval(w_i, w_o) * light_sample.illum
					* std::abs(glm::dot(light_sample.dir, dg.normal));
			}
		} else if (ray.depth < 6) {
			if (dg.brdf->bxdf_type() & BRDFType::Reflection) {
				const BxDFSample f = dg.brdf->sample(w_o, dummy_samples);
				if (f.pdf != 0.f) {
					Ray refl_ray(dg.point, dg.from_shading(f.w_i), 0.001);
					refl_ray.depth = ray.depth + 1;
					lighting = f.color * integrate(refl_ray);
				}
			}
			if (dg.brdf->bxdf_type() & BRDFType::Transmission) {
				// todo
			}
		}
		return lighting;
	}
	return glm::vec3(0);
}

}


