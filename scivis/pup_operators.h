#pragma once

#include <glm/glm.hpp>
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
inline void operator|(PUP::er &p, glm::uvec3 &v) {
	p | v.x;
	p | v.y;
	p | v.z;
}

