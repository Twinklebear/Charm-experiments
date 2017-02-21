#include "whitted_integrator.h"
#include "diff_geom.h"
#include "light.h"
#include "brdf.h"

namespace pt {

WhittedIntegrator::WhittedIntegrator(const glm::vec3 &background, Scene scene)
	: background(background), scene(std::move(scene))
{}
glm::vec3 WhittedIntegrator::integrate(Ray &ray) const {
	const float dummy_samples[2] = {0};
	DifferentialGeometry dg;
	if (scene.intersect(ray, dg)) {
		glm::vec3 lighting(0);
		dg.orthonormalize();
		const glm::vec3 w_o = dg.to_shading(-ray.dir);
		if (!(dg.brdf->bxdf_type() & BRDFType::Specular)) {
			for (const auto &l : scene.lights) {
				const LightSample light_sample = l->incident(dg.point);
				const glm::vec3 w_i = dg.to_shading(light_sample.dir);

				// TODO: Should detect transmission vs. reflection case and request appropriate brdf
				if (glm::dot(light_sample.dir, dg.normal) > 0.0 && !light_sample.occluded(scene)) {
					// note: no division by pdf since it's 1 for the delta light
					lighting += dg.brdf->eval(w_i, w_o) * light_sample.illum
						* std::abs(glm::dot(light_sample.dir, dg.normal));
				}
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
				// TODO
			}
		}
		return lighting;
	}
	return background;
}

}


