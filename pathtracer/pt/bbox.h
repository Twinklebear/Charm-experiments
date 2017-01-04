#pragma once

#include <glm/glm.hpp>
#include "ray.h"

namespace pt {

struct BBox {
	glm::vec3 min, max;

	inline BBox(const glm::vec3 &min, const glm::vec3 &max) : min(min), max(max) {}
	inline const glm::vec3& operator[](const size_t i) const {
		return i == 0 ? min : max;
	}
	inline glm::vec3& operator[](const size_t i) {
		return i == 0 ? min : max;
	}
	// Fast box intersection
	inline bool intersect(const Ray &r, const glm::vec3 &inv_dir, const std::array<int, 3> &neg_dir,
			float *t_min = nullptr, float *t_max = nullptr) const {
		//Check X & Y intersection
		float tmin = ((*this)[neg_dir[0]].x - r.origin.x) * inv_dir.x;
		float tmax = ((*this)[1 - neg_dir[0]].x - r.origin.x) * inv_dir.x;
		float tymin = ((*this)[neg_dir[1]].y - r.origin.y) * inv_dir.y;
		float tymax = ((*this)[1 - neg_dir[1]].y - r.origin.y) * inv_dir.y;
		if (tmin > tymax || tymin > tmax){
			return false;
		}
		if (tymin > tmin){
			tmin = tymin;
		}
		if (tymax < tmax){
			tmax = tymax;
		}

		//Check Z intersection
		float tzmin = ((*this)[neg_dir[2]].z - r.origin.z) * inv_dir.z;
		float tzmax = ((*this)[1 - neg_dir[2]].z - r.origin.z) * inv_dir.z;
		if (tmin > tzmax || tzmin > tmax){
			return false;
		}
		if (tzmin > tmin){
			tmin = tzmin;
		}
		if (tzmax < tmax){
			tmax = tzmax;
		}
		if (tmin < r.t_max && tmax > r.t_min) {
			if (t_min) {
				*t_min = tmin;
			}
			if (t_max) {
				*t_max = tmax;
			}
			return true;
		}
		return false;
	}
};

}

