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
				// TODO: Doing this occlusion test needs to potentially ship a ray off to other nodes,
				// who will then be responsible for shading it and sending the color back to the owner.
				// TODO: The occlusion test should check against hitting our local geometry first,
				// which more often than not will likely be the case (I'd guess?). Only after missing
				// our local geometry we then need to see if it should be sent to other regions in the
				// region BVH
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
					// TODO: If we have no guarantees on whether the region bounds overlap or
					// not this reflection ray can test our local geometry, then see if we found
					// a hit nearer than the first region bounds we hit in the BVH. It might be
					// worth storing a flag on the BVH to check if any of the leaf nodes (the regions)
					// overlap each other. Shouldn't this be done by the BVH for us though, since the t_max
					// will be set and so when we traverse the BVH we will find no hits.
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


