#pragma once

#include <vector>
#include <memory>
#include "ray.h"
#include "diff_geom.h"
#include "geometry.h"

namespace pt {

class Light;
/* Describes the geometry in the scene being rendered
 */
struct Scene {
	std::vector<std::shared_ptr<Geometry>> geometry;
	std::vector<std::shared_ptr<Light>> lights;

	Scene(std::vector<std::shared_ptr<Geometry>> geom, std::vector<std::shared_ptr<Light>> lights);
	// Find the object hit by the ray in the scene
	bool intersect(Ray &ray, DifferentialGeometry &dg) const;
};

}

