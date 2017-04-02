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

