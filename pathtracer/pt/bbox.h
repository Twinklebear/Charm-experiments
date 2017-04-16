#pragma once

#include <iostream>
#include <limits>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "ray.h"

namespace pt {

struct BBox {
	glm::vec3 min, max;

	inline BBox(const glm::vec3 &min = glm::vec3(std::numeric_limits<float>::infinity()),
			const glm::vec3 &max = -glm::vec3(std::numeric_limits<float>::infinity()))
		: min(min), max(max)
	{}
	inline const glm::vec3& operator[](const size_t i) const {
		return i == 0 ? min : max;
	}
	inline glm::vec3& operator[](const size_t i) {
		return i == 0 ? min : max;
	}
	// Check if this box overlaps the one passed
	inline bool overlaps(const BBox &b) const {
		return max.x >= b.min.x && min.x <= b.max.x
			&& max.y >= b.min.y && min.y <= b.max.y
			&& max.z >= b.min.z && min.z <= b.max.z;
	}
	// Extend the box to include the passed point
	inline void extend(const glm::vec3 &pt) {
		max = glm::max(max, pt);
		min = glm::min(min, pt);
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

inline std::ostream& operator<<(std::ostream &os, const pt::BBox &b) {
	os << "BBox { " << glm::to_string(b.min)
		<< ", " << glm::to_string(b.max) << "\n";
	return os;
}

