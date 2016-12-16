#include "scene.h"

void ortho_basis(std::array<vec3f, 3> &basis, const vec3f &n) {
	basis[2] = n;
	basis[1] = 0.f;

	if (n.x < 0.6f && n.x > -0.6f){
		basis[1].x = 1.f;
	} else if (n.y < 0.6f && n.y > -0.6f){
		basis[1].y = 1.f;
	} else if (n.z < 0.6f && n.z > -0.6f){
		basis[1].z = 1.f;
	} else {
		basis[1].x = 1.f;
	}
	basis[0] = normalized(cross(basis[1], basis[2]));
	basis[1] = normalized(cross(basis[2], basis[0]));
}

void Sphere::intersect(Isect &isect, const Ray &ray) const {
	vec3f rs = ray.origin - center;

	const float b = dot(rs, ray.dir);
	const float c = dot(rs, rs) - radius * radius;
	const float discrim = b * b - c;
	if (discrim > 0.f){
		// Note: we will never hit the backface of the sphere
		const float t = -b - std::sqrt(discrim);
		if (t > 0.f && t < isect.t){
			isect.t = t;
			isect.hit = 1;

			isect.p = ray.origin + ray.dir * t;
			isect.n = normalized(isect.p - center);
		}
	}
}

void Plane::intersect(Isect &isect, const Ray &ray) const {
	const float d = -dot(p, n);
	const float v = dot(ray.dir, n);
	if (std::abs(v) < 1e-6f){
		return;
	}

	const float t = -(dot(ray.origin, n) + d) / v;
	if (t > 0.f && t < isect.t){
		isect.t = t;
		isect.hit = 1;

		isect.p = ray.origin + ray.dir * t;
		isect.n = n;
	}
}

