#pragma once

#include <memory>
#include "ray.h"
#include "bbox.h"
#include "brdf.h"
#include "diff_geom.h"

namespace pt {

class Geometry {
protected:
	std::shared_ptr<BxDF> brdf;

public:
	Geometry(std::shared_ptr<BxDF> &brdf);
	virtual ~Geometry();
	virtual bool intersect(Ray &ray) const = 0;
	virtual void get_shading_info(const Ray &ray, DifferentialGeometry &dg) const = 0;
	virtual BBox bounds() const = 0;
};

// Compute an orthonormal coordinate system with e1 as one of the axes
void coordinate_system(const glm::vec3 &e1, glm::vec3 &e2, glm::vec3 &e3);

}

