#include <numeric>
#include <algorithm>
#include "scene.h"

namespace pt {

Scene::Scene(std::vector<std::shared_ptr<Geometry>> geom, std::vector<std::shared_ptr<Light>> lights)
	: geometry(geom), lights(lights)
{
	std::vector<const Geometry*> g;
	std::transform(geometry.begin(), geometry.end(), std::back_inserter(g),
			[](std::shared_ptr<Geometry> &x) { return x.get(); });
	bvh = BVH(g, 2);
}
bool Scene::intersect(Ray &ray, DifferentialGeometry &dg) const {
	return bvh.intersect(ray, dg);
}

}

