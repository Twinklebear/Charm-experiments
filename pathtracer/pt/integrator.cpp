#include <bitset>
#include <iostream>
#include <glm/ext.hpp>
#include "integrator.h"

namespace pt {

BVHTraversalState::BVHTraversalState() : current(0), bitstack(0) {}

HitInfo::HitInfo() : hit(false), hit_owner(-1), hit_object(-1) {}

ActiveRay::ActiveRay(const Ray &r, const uint64_t owner_id, const uint64_t tile,
		const uint64_t pixel, const glm::vec3 &throughput)
	: type(PRIMARY), ray(r), color(0), throughput(throughput), owner_id(owner_id),
	tile(tile), pixel(pixel), shadow_children(0)
{}
ActiveRay* ActiveRay::shadow(const Ray &r, const ActiveRay &parent) {
	ActiveRay *ar = new ActiveRay(r, parent.owner_id, parent.tile,
			parent.pixel, glm::vec3(0));
	ar->type = SHADOW;
	return ar;
}
ActiveRay* ActiveRay::secondary(const Ray &r, const ActiveRay &parent) {
	ActiveRay *ar = new ActiveRay(r, parent.owner_id, parent.tile,
			parent.pixel, parent.throughput);
	ar->shadow_children = parent.shadow_children;
	ar->type = SECONDARY;
	return ar;
}

IntersectionResult::IntersectionResult() : any_hit(false) {}

}

std::ostream& operator<<(std::ostream &os, const pt::RAY_TYPE &t) {
	switch (t) {
		case pt::RAY_TYPE::PRIMARY: os << "RAY_TYPE::PRIMARY"; break;
		case pt::RAY_TYPE::SHADOW: os << "RAY_TYPE::SHADOW"; break;
		case pt::RAY_TYPE::SECONDARY: os << "RAY_TYPE::SECONDARY"; break;
	}
	return os;
}
std::ostream& operator<<(std::ostream &os, const pt::ActiveRay &r) {
	os << "ActiveRay {\ntype: " << r.type
		<< "\n\tray: " << r.ray
		<< "\n\ttraversal: " << r.traversal
		<< "\n\thit: " << r.hit_info.hit
		<< "\n\thit owner: " << r.hit_info.hit_owner
		<< "\n\thit object: " << r.hit_info.hit_object
		<< "\n\tcolor: " << glm::to_string(r.color)
		<< "\n\tthroughput: " << glm::to_string(r.throughput)
		<< "\n\towner_id: " << r.owner_id
		<< "\n\ttile: " << r.tile
		<< "\n\tpixel: " << r.pixel
		<< "\n\tchildren: " << r.shadow_children
		<< "\n}";
	return os;
}
std::ostream& operator<<(std::ostream &os, const pt::BVHTraversalState &s) {
	os << "BVHTraversalState { current: " << s.current << ", bitstack: "
		<< std::bitset<sizeof(size_t)>(s.bitstack) << " }";
	return os;
}

