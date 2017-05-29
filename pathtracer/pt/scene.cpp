#include <numeric>
#include <algorithm>
#include "scene.h"

namespace pt {

Scene::Scene(std::vector<std::shared_ptr<Geometry>> geom, std::vector<std::shared_ptr<Light>> lights,
		const BVH *bvh)
	: geometry(geom), lights(lights), bvh(bvh)
{}
bool Scene::intersect(Ray &ray, DifferentialGeometry &dg) const {
	const glm::vec3 inv_dir = 1.f / ray.dir;
	const std::array<int, 3> neg_dir{ray.dir.x < 0 ? 1 : 0,
		ray.dir.y < 0 ? 1 : 0, ray.dir.z < 0 ? 1 : 0
	};
	return std::accumulate(geometry.begin(), geometry.end(), false,
		[&](const bool &hit, const std::shared_ptr<Geometry> &g) {
			if (g->bounds().fast_intersect(ray, inv_dir, neg_dir)) {
				return g->intersect(ray, dg) || hit;
			} else {
				return hit;
			}
		});
}

}

