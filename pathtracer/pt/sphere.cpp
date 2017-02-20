#include <cmath>
#include <glm/ext.hpp>
#include "sphere.h"

namespace pt {

Sphere::Sphere(const glm::vec3 &center, const float radius) : center(center), radius(radius) {}
bool Sphere::intersect(Ray &ray, DifferentialGeometry &dg) const {
	const float a = glm::length2(ray.dir);
	const glm::vec3 oc = ray.origin - center;
	const float b = 2.0 * glm::dot(ray.dir, oc);
	const float c = glm::length2(oc) - radius * radius;
	const float discrim = b * b - 4.0 * a * c;
	if (discrim > 0.f){
		float t = (-b - std::sqrt(discrim)) / (2.0 * a);
		if (t > ray.t_min && t < ray.t_max){
			ray.t_max = t;
			dg.point = ray.origin + ray.dir * t;
			dg.normal = glm::normalize(dg.point - center);
			// Compute the tangent and bitangent
			// Note: b/c none of the materials depend on the tanget/bitangent
			// orientation being consistent this should be fine, we just need some
			// sort of shading coordinate frame
			coordinate_system(dg.normal, dg.tangent, dg.bitangent);
			return true;
		} else {
			t = (-b + std::sqrt(discrim)) / (2.0 * a);
			if (t > ray.t_min && t < ray.t_max){
				ray.t_max = t;
				dg.point = ray.origin + ray.dir * t;
				dg.normal = glm::normalize(dg.point - center);
				coordinate_system(dg.normal, dg.tangent, dg.bitangent);
				return true;
			}
		}
	}
	return false;
}

}

