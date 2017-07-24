#include <numeric>
#include <algorithm>
#include "scene.h"

namespace pt {

Scene::Scene(std::vector<std::shared_ptr<Geometry>> geom, std::vector<std::shared_ptr<Light>> lights,
		const BVH *bvh)
	: geometry(geom), lights(lights), bvh(bvh)
{}
bool Scene::intersect(ActiveRay &ray) const {
	const glm::vec3 inv_dir = 1.f / ray.ray.dir;
	const std::array<int, 3> neg_dir{
		ray.ray.dir.x < 0 ? 1 : 0,
		ray.ray.dir.y < 0 ? 1 : 0,
		ray.ray.dir.z < 0 ? 1 : 0
	};
	bool hit = false;
	for (size_t i = 0; i < geometry.size(); ++i) {
		const std::shared_ptr<Geometry> &g = geometry[i];
		if (g->bounds().fast_intersect(ray.ray, inv_dir, neg_dir)) {
			if (g->intersect(ray.ray)) {
				hit = true;
				ray.hit_info.hit = true;
				ray.hit_info.hit_object = i;
			}
		}
	}
	return hit;
}
bool Scene::intersect(Ray &ray, DifferentialGeometry &dg) const {
	const glm::vec3 inv_dir = 1.f / ray.dir;
	const std::array<int, 3> neg_dir{ray.dir.x < 0 ? 1 : 0,
		ray.dir.y < 0 ? 1 : 0, ray.dir.z < 0 ? 1 : 0
	};
	return std::accumulate(geometry.begin(), geometry.end(), false,
		[&](const bool &hit, const std::shared_ptr<Geometry> &g) {
			if (g->bounds().fast_intersect(ray, inv_dir, neg_dir)) {
				if (g->intersect(ray)) {
					g->get_shading_info(ray, dg);
				}
				return true;
			} else {
				return hit;
			}
		});
}

}

