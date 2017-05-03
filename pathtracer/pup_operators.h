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


