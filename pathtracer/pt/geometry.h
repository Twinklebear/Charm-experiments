#pragma once

#include "ray.h"
#include "diff_geom.h"

namespace pt {

class Geometry {
public:
	virtual ~Geometry(){}
	virtual bool intersect(Ray &ray, DifferentialGeometry &dg) const = 0;
};

}

