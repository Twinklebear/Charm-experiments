#include <numeric>
#include <algorithm>
#include "scene.h"

namespace pt {

Scene::Scene(std::vector<std::shared_ptr<Geometry>> geom, std::vector<std::shared_ptr<Light>> lights)
	: geometry(geom), lights(lights)
{}
bool Scene::intersect(Ray &ray, DifferentialGeometry &dg) const {
	return std::accumulate(geometry.begin(), geometry.end(), false,
		[&](const bool &hit, const std::shared_ptr<Geometry> &g) {
			return g->intersect(ray, dg) || hit;
		});
}

}

