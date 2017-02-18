#pragma once

#include <vector>
#include <memory>
#include "ray.h"
#include "diff_geom.h"
#include "geometry.h"

namespace pt {

/* Describes the geometry in the scene being rendered
 */
class Scene {
	std::vector<std::shared_ptr<Geometry>> geometry;

public:
	Scene(std::vector<std::shared_ptr<Geometry>> geom);
	// Find the object hit by the ray in the scene
	bool intersect(Ray &ray, DifferentialGeometry &dg) const;
};

}

