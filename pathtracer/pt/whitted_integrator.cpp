#include "whitted_integrator.h"
#include "diff_geom.h"
#include "light.h"

namespace pt {

WhittedIntegrator::WhittedIntegrator(Scene scene) : scene(std::move(scene)) {}
glm::vec3 WhittedIntegrator::integrate(Ray &ray) const {
	DifferentialGeometry dg;
	if (scene.intersect(ray, dg)) {
		// TODO: Don't do a hard-coded Blinn-Phong lighting model
		const glm::vec3 ambient_light(0.1);
		const DirectionalLight dir_light(glm::vec3(-1), glm::vec3(0.8));

		const glm::vec3 diffuse_color(0.1, 0.1, 0.8);
		const glm::vec3 specular_color(0.2);
		const float spec_exponent = 50;

		glm::vec3 lighting = ambient_light * diffuse_color;
		const LightSample light_sample = dir_light.incident(dg.point);
		if (glm::dot(light_sample.dir, dg.normal) > 0.0 && !light_sample.occluded(scene)) {
			const glm::vec3 half_vec = glm::normalize(light_sample.dir - ray.dir);
			lighting += diffuse_color * light_sample.illum * glm::dot(light_sample.dir, dg.normal)
				+ specular_color * light_sample.illum * std::pow(glm::dot(dg.normal, half_vec), spec_exponent);
		}
		return lighting;
	}
	return glm::vec3(0);
}

}


