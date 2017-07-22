#pragma once

#include <iostream>
#include <limits>
#include <array>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "ray.h"

namespace pt {

struct BBox {
	glm::vec3 min, max;

	inline BBox(const glm::vec3 &min, const glm::vec3 &max)
		: min(min), max(max)
	{}
	inline BBox() : min(glm::vec3(std::numeric_limits<float>::infinity())),
		max(-glm::vec3(std::numeric_limits<float>::infinity()))
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
		min = glm::min(min, pt);
		max = glm::max(max, pt);
	}
	// Union this box with the other one
	inline void box_union(const BBox &b) {
		min = glm::min(min, b.min);
		max = glm::max(max, b.max);
	}
	// Intersect this box with the other one
	inline static BBox box_union(const BBox &a, const BBox &b) {
		return BBox(glm::min(a.min, b.min), glm::max(a.max, b.max));
	}
	// Intersect this box with the other one
	inline static BBox intersect(const BBox &a, const BBox &b) {
		return BBox(glm::max(a.min, b.min), glm::min(a.max, b.max));
	}
	inline int max_extent() const {
		const glm::vec3 d = max - min;
		if (d.x >= d.y && d.x >= d.z) {
			return 0;
		} else if (d.y >= d.z) {
			return 1;
		}
		return 2;
	}
	inline glm::vec3 center() const {
		return glm::lerp(min, max, 0.5f);
	}
	inline float surface_area() const {
		const glm::vec3 d = max - min;
		return 2.f * (d.x * d.y + d.x * d.z + d.y * d.z);
	}
	// Fast box intersection
	inline bool fast_intersect(const Ray &r, const glm::vec3 &inv_dir, const std::array<int, 3> &neg_dir,
			float *t_min = nullptr, float *t_max = nullptr) const {
		// Check X & Y intersection
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

		// Check Z intersection
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
	// Intersect the ray with the box and update the ray's t_max
	inline bool intersect(Ray &r, const glm::vec3 &inv_dir, const std::array<int, 3> &neg_dir) const {
		float t_min = r.t_min;
		float t_max = r.t_max;
		if (fast_intersect(r, inv_dir, neg_dir, &t_min, &t_max)) {
			if (t_min > r.t_min) {
				r.t_max = t_min;
				return true;
			} else if (t_max < r.t_max) {
				r.t_max = t_max;
				return true;
			}
		}
		return false;
	}
};

}

inline std::ostream& operator<<(std::ostream &os, const pt::BBox &b) {
	os << "BBox {\n\tmin: " << glm::to_string(b.min)
		<< ",\n\tmax: " << glm::to_string(b.max) << "\n}";
	return os;
}

