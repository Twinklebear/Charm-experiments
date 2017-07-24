#include "plane.h"

namespace pt {

Plane::Plane(const glm::vec3 &center, const glm::vec3 &normal, float half_length,
		std::shared_ptr<BxDF> &brdf)
	: Geometry(brdf), center(center), normal(glm::normalize(normal)), half_length(half_length)
{}
bool Plane::intersect(Ray &ray) const {
	const float d = -glm::dot(center, normal);
	const float v = glm::dot(ray.dir, normal);
	if (std::abs(v) < 1e-6f){
		return false;
	}

	const float t = -(glm::dot(ray.origin, normal) + d) / v;
	if (t > ray.t_min && t < ray.t_max){
		const glm::vec3 pt = ray.origin + ray.dir * t;
		// Compute the tangent and bitangent
		glm::vec3 tan, bitan;
		coordinate_system(normal, tan, bitan);
		if (std::abs(glm::dot(pt - center, tan)) <= half_length
			&& std::abs(glm::dot(pt - center, bitan)) <= half_length)
		{
			ray.t_max = t;
			return true;
		} else {
			return false;
		}
	}
	return false;
}
void Plane::get_shading_info(const Ray &ray, DifferentialGeometry &dg) const {
	dg.point = ray.origin + ray.dir * ray.t_max;
	dg.normal = normal;
	dg.brdf = brdf.get();
	coordinate_system(normal, dg.tangent, dg.bitangent);
}
BBox Plane::bounds() const {
	glm::vec3 tan, bitan;
	coordinate_system(normal, tan, bitan);
	const glm::vec3 min_pt = center - half_length * tan - half_length * bitan - 0.001 * normal;
	const glm::vec3 max_pt = center + half_length * tan + half_length * bitan + 0.001 * normal;
	return BBox(glm::min(min_pt, max_pt), glm::max(min_pt, max_pt));
}

}

