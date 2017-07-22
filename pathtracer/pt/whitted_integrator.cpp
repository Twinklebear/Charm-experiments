#include "whitted_integrator.h"
#include "diff_geom.h"
#include "light.h"
#include "brdf.h"

namespace pt {

WhittedIntegrator::WhittedIntegrator(const glm::vec3 &background, Scene scene)
	: background(background), scene(std::move(scene))
{}
IntersectionResult WhittedIntegrator::integrate(ActiveRay &ray) const {
	IntersectionResult result;

	const float dummy_samples[2] = {0};
	DifferentialGeometry dg;
	if (scene.intersect(ray.ray, dg)) {
		result.any_hit = true;
		glm::vec3 lighting(0);
		dg.orthonormalize();
		const glm::vec3 w_o = dg.to_shading(-ray.ray.dir);
		if (!(dg.brdf->bxdf_type() & BRDFType::Specular)) {
			for (const auto &l : scene.lights) {
				const LightSample light_sample = l->incident(dg.point);
				const glm::vec3 w_i = dg.to_shading(light_sample.dir);
				const glm::vec3 color = dg.brdf->eval(w_i, w_o) * light_sample.illum 
					* std::max(glm::dot(light_sample.dir, dg.normal), 0.f);
				if (color != glm::vec3(0.f)) {
					result.shadow = std::unique_ptr<ActiveRay>(ActiveRay::shadow(light_sample.occlusion_ray, ray));
					for (size_t i = 0; i < 3; ++ i) {
						result.shadow->color[i] = color[i];
					}
				}
			}
		} else if (ray.ray.depth < 6) {
			// TODO: We actually need to create and ship off an ActiveRay as a secondary ray.
			if (dg.brdf->bxdf_type() & BRDFType::Reflection) {
				const BxDFSample f = dg.brdf->sample(w_o, dummy_samples);
				if (f.pdf != 0.f && f.color != glm::vec3(0.f)) {
					Ray refl_ray(dg.point, dg.from_shading(f.w_i), 0.001);
					refl_ray.depth = ray.ray.depth + 1;
					result.secondary = std::unique_ptr<ActiveRay>(ActiveRay::secondary(refl_ray, ray));
					for (size_t i = 0; i < 3; ++ i) {
						result.secondary->color[i] = f.color[i];
					}
				}
			}
			if (dg.brdf->bxdf_type() & BRDFType::Transmission) {
				// TODO
			}
		}
	}
	return result;
}
bool WhittedIntegrator::occluded(ActiveRay &ray) const {
	DifferentialGeometry dg;
	return scene.intersect(ray.ray, dg);
}

}


