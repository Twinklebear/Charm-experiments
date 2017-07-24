#pragma once

#include <glm/glm.hpp>
#include <pup.h>
#include "pt/pt.h"

template<typename T>
inline void operator|(PUP::er &p, glm::tvec3<T, (glm::precision)0> &v) {
	p | v.x;
	p | v.y;
	p | v.z;
}
template<typename T>
inline void operator|(PUP::er &p, glm::tvec4<T, (glm::precision)0> &v) {
	p | v.x;
	p | v.y;
	p | v.z;
	p | v.w;
}
inline void operator|(PUP::er &p, pt::BBox &bounds) {
	p | bounds.min;
	p | bounds.max;
}
inline void operator|(PUP::er &p, pt::Ray &ray) {
	p | ray.origin;
	p | ray.dir;
	p | ray.t_min;
	p | ray.t_max;
	p | ray.depth;
}
inline void operator|(PUP::er &p, pt::HitInfo &hit) {
	p | hit.hit;
	p | hit.hit_owner;
	p | hit.hit_object;
}
inline void operator|(PUP::er &p, pt::ActiveRay &ray) {
	int ray_type = ray.type;
	p | ray_type;
	ray.type = static_cast<pt::RAY_TYPE>(ray_type);
	p | ray.ray;

	p | ray.traversal.current;
	p | ray.traversal.bitstack;

	p | ray.hit_info;

	p | ray.color;
	p | ray.throughput;
	p | ray.owner_id;
	p | ray.tile;
	p | ray.pixel;
	p | ray.children;
}

