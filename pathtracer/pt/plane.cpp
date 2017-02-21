#include "plane.h"

namespace pt {

Plane::Plane(const glm::vec3 &center, const glm::vec3 &normal, std::shared_ptr<BxDF> &brdf)
	: Geometry(brdf), center(center), normal(normal)
{}
bool Plane::intersect(Ray &ray, DifferentialGeometry &dg) const {
	const float d = -glm::dot(center, normal);
	const float v = glm::dot(ray.dir, normal);
	if (std::abs(v) < 1e-6f){
		return false;
	}

	const float t = -(glm::dot(ray.origin, normal) + d) / v;
	if (t > ray.t_min && t < ray.t_max){
		ray.t_max = t;
		dg.point = ray.origin + ray.dir * t;
		dg.normal = normal;
		dg.brdf = brdf.get();
		// Compute the tangent and bitangent
		coordinate_system(normal, dg.tangent, dg.bitangent);
		return true;
	}
	return false;
}

}

