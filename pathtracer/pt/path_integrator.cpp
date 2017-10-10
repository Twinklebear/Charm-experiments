#include <iostream>
#include <glm/ext.hpp>
#include "path_integrator.h"
#include "diff_geom.h"
#include "light.h"
#include "brdf.h"

namespace pt {

PathIntegrator::PathIntegrator(const glm::vec3 &background, std::shared_ptr<Scene> s)
	: rng(std::random_device()()), light_sample(0, s->lights.size() - 1),
	background(background), scene(s)
{}
glm::vec3 PathIntegrator::integrate(Ray &start) {
	// TODO: For simple testing maybe switch this to still be usable image-parallel?
	// It can just combine calls to intersect and integrate to shade the initial camera ray
	DifferentialGeometry dg;
	glm::vec3 illum(0), path_throughput(1);
	Ray ray = start;
	float samples[2] = {0};
	for (size_t i = 0; i < 10; ++i) {
		if (scene->intersect(ray, dg)) {
			dg.orthonormalize();
			const glm::vec3 w_o = glm::normalize(dg.to_shading(-ray.dir));

			// Do direct light sampling
			if (!(dg.brdf->bxdf_type() & BRDFType::Specular)) {
				const Light *l = scene->lights[light_sample(rng)].get();
				const LightSample light_sample = l->incident(dg.point);
				const glm::vec3 w_i = dg.to_shading(light_sample.dir);

				if (glm::dot(light_sample.dir, dg.normal) > 0.0 && !light_sample.occluded(*scene)) {
					// note: no division by pdf since it's 1 for the delta light
					// TODO: We should divide by the probability of picking the light we chose though
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

			ray = Ray(dg.point, glm::normalize(dg.from_shading(f.w_i)), 0.001);
			ray.depth = i;
			path_throughput *= f.color * std::abs(glm::dot(ray.dir, dg.normal)) / f.pdf;
		} else {
			illum += path_throughput * background;
			break;
		}
	}
	return illum;
}
IntersectionResult PathIntegrator::integrate(const ActiveRay &ray) {
	IntersectionResult result;

	DifferentialGeometry dg;
	scene->geometry[ray.hit_info.hit_object]->get_shading_info(ray.ray, dg);
	dg.orthonormalize();
	const glm::vec3 w_o = glm::normalize(dg.to_shading(-ray.ray.dir));

	// Do direct light sampling
	if (!(dg.brdf->bxdf_type() & BRDFType::Specular)) {
		const Light *l = scene->lights[light_sample(rng)].get();
		const LightSample light_sample = l->incident(dg.point);
		const glm::vec3 w_i = dg.to_shading(light_sample.dir);

		// note: no division by pdf since it's 1 for the delta light
		// TODO: We should divide by the probability of picking the light we chose though
		const glm::vec3 f = dg.brdf->eval(w_i, w_o);
		if (glm::dot(light_sample.dir, dg.normal) > 0.0 && f != glm::vec3(0.f)) {
			result.shadow = std::unique_ptr<ActiveRay>(ActiveRay::shadow(light_sample.occlusion_ray, ray));
			result.shadow->color = ray.throughput * f * light_sample.illum
				* std::abs(glm::dot(light_sample.dir, dg.normal));
		}
	}

	// Spawn the secondary ray to continue the path if we're not at our depth limit
	if (ray.ray.depth < 10) {
		const float samples[2] = { brdf_sample(rng), brdf_sample(rng) };
		const BxDFSample f = dg.brdf->sample(w_o, samples);
		if (f.pdf != 0.f && f.color != glm::vec3(0.f)) {
			Ray secondary_ray = Ray(dg.point, glm::normalize(dg.from_shading(f.w_i)), 0.001);
			secondary_ray.depth = ray.ray.depth + 1;

			result.secondary = std::unique_ptr<ActiveRay>(ActiveRay::secondary(secondary_ray, ray));
			result.secondary->throughput = ray.throughput * f.color
				* std::abs(glm::dot(secondary_ray.dir, dg.normal)) / f.pdf;
		}
	}
	return result;
}
bool PathIntegrator::occluded(ActiveRay &ray) {
	return scene->intersect(ray);
}

}



