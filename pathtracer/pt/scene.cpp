#include <algorithm>
#include "scene.h"

namespace pt {

Scene::Scene(std::vector<std::unique_ptr<Geometry>> geom) : geometry(std::move(geom)) {}
bool Scene::intersect(Ray &ray, DifferentialGeometry &dg) const {
	return std::accumulate(geometry.begin(), geometry.end(), false,
		[&](const bool &hit, const std::unique_ptr<Geometry> &g) {
			return g->intersect(ray, dg) || hit;
		});
}

}

