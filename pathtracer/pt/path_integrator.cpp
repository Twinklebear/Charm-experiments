#include "path_integrator.h"
#include "diff_geom.h"
#include "light.h"
#include "brdf.h"

namespace pt {

PathIntegrator::PathIntegrator(const glm::vec3 &background, Scene s)
	: background(background), scene(std::move(s)), rng(std::random_device()()),
	light_sample(0, scene.lights.size() - 1)
{}
glm::vec3 PathIntegrator::integrate(Ray &start) const {
	DifferentialGeometry dg;
	glm::vec3 illum(0), path_throughput(1);
	Ray ray = start;
	float samples[2] = {0};
	for (size_t i = 0; i < 10; ++i) {
		if (scene.intersect(ray, dg)) {
			dg.orthonormalize();
			const glm::vec3 w_o = dg.to_shading(-ray.dir);

			// Do direct light sampling
			if (!(dg.brdf->bxdf_type() & BRDFType::Specular)) {
				const Light *l = scene.lights[light_sample(rng)].get();
				const LightSample light_sample = l->incident(dg.point);
				const glm::vec3 w_i = dg.to_shading(light_sample.dir);
				if (glm::dot(light_sample.dir, dg.normal) > 0.0 && !light_sample.occluded(scene)) {
					// note: no division by pdf since it's 1 for the delta light
					illum += path_throughput * dg.brdf->eval(w_i, w_o) * light_sample.illum
						* std::abs(glm::dot(light_sample.dir, dg.normal));
				}
			}

			// Sample BRDF for next ray direction
			samples[0] = brdf_sample(rng);
			samples[1] = brdf_sample(rng);
			const BxDFSample f = dg.brdf->sample(w_o, samples);
			if (f.pdf == 0.f || f.color == glm::vec3(0.f)) {
				break;
			}

			ray = Ray(dg.point, dg.from_shading(f.w_i), 0.001);
			ray.depth = i;
			path_throughput = path_throughput * f.color * std::abs(glm::dot(ray.dir, dg.normal)) / f.pdf;
		} else {
			illum += path_throughput * background;
			break;
		}
	}
	return illum;
}

}



