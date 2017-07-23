#include <bitset>
#include <iostream>
#include <glm/ext.hpp>
#include "integrator.h"

namespace pt {

BVHTraversalState::BVHTraversalState() : current(0), bitstack(0) {}

ActiveRay::ActiveRay(const Ray &r, const uint64_t owner_id, const uint64_t tile,
		const uint64_t pixel)
	: type(PRIMARY), ray(r), color(glm::vec3(0), r.t_max), owner_id(owner_id),
	tile(tile), pixel(pixel), children(0)
{}
ActiveRay* ActiveRay::shadow(const Ray &r, const ActiveRay &parent) {
	ActiveRay *ar = new ActiveRay(r, parent.owner_id, parent.tile, parent.pixel);
	ar->children = 0;
	ar->type = SHADOW;
	ar->color.w = parent.ray.t_max;
	ar->ray = r;
	return ar;
}
ActiveRay* ActiveRay::secondary(const Ray &r, const ActiveRay &parent) {
	ActiveRay *ar = new ActiveRay(r, parent.owner_id, parent.tile, parent.pixel);
	ar->children = 0;
	ar->type = SECONDARY;
	ar->color.w = parent.ray.t_max;
	ar->ray = r;
	return ar;
}

IntersectionResult::IntersectionResult() : any_hit(false) {}

}

std::ostream& operator<<(std::ostream &os, const pt::ActiveRay &r) {
	os << "ActiveRay {\ntype: ";
	switch (r.type) {
		case pt::RAY_TYPE::PRIMARY: os << "PRIMARY\n"; break;
		case pt::RAY_TYPE::SHADOW: os << "SHADOW\n"; break;
		case pt::RAY_TYPE::SECONDARY: os << "SECONDARY\n"; break;
	}
	os << "ray: " << r.ray << "\ntraversal: " << r.traversal
		<< "\ncolor: " << glm::to_string(r.color)
		<< "\nowner_id: " << r.owner_id
		<< "\ntile: " << r.tile
		<< "\npixel: " << r.pixel
		<< "\nchildren: " << r.children
		<< "\n}";
	return os;
}
std::ostream& operator<<(std::ostream &os, const pt::BVHTraversalState &s) {
	os << "BVHTraversalState { current: " << s.current << ", bitstack: "
		<< std::bitset<sizeof(size_t)>(s.bitstack) << " }";
	return os;
}

