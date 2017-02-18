#include "whitted_integrator.h"
#include "diff_geom.h"

namespace pt {

WhittedIntegrator::WhittedIntegrator(const std::vector<Sphere> &scene) : scene(scene) {}
glm::vec3 WhittedIntegrator::integrate(Ray &ray) const {
	DifferentialGeometry dg;
	bool hit = false;
	for (const auto &s : scene) {
		hit = s.intersect(ray, dg) || hit;
	}
	if (hit) {
		// TODO: Don't do a hard-coded Blinn-Phong lighting model
		const glm::vec3 ambient_light(0.1);
		const glm::vec3 light_dir = glm::normalize(glm::vec3(1, 1, 1));
		const glm::vec3 light_color(0.8);
		
		const glm::vec3 half_vec = glm::normalize(light_dir - ray.dir);
		const glm::vec3 diffuse_color(0.1, 0.1, 0.8);
		const glm::vec3 specular_color(0.2);
		const float spec_exponent = 50;

		glm::vec3 lighting = ambient_light * diffuse_color;
		if (glm::dot(light_dir, dg.normal) > 0.0) {
			lighting += diffuse_color * light_color * glm::dot(light_dir, dg.normal)
				+ specular_color * light_color * std::pow(glm::dot(dg.normal, half_vec), spec_exponent);
		}
		return lighting;
	}
	return glm::vec3(0);
}

}


