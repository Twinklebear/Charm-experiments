#pragma once

#include <glm/glm.hpp>
#include <pup.h>
#include "sv/scivis.h"

inline void operator|(PUP::er &p, sv::VolumeDType &dtype) {
	if (p.isUnpacking()) {
		int t = 0;
		p | t;
		dtype = (sv::VolumeDType)t;
	} else {
		int t = dtype;
		p | t;
	}
}
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

