#include <bitset>
#include "integrator.h"

namespace pt {

ActiveRay::ActiveRay(const Ray &r, const uint64_t owner_id, const uint64_t tile,
		const uint64_t pixel)
	: type(PRIMARY), ray(r), owner_id(owner_id), tile(tile), pixel(pixel),
	children(0)
{}
ActiveRay* ActiveRay::shadow(const Ray &r, const ActiveRay &parent) {
	ActiveRay *ar = new ActiveRay(parent);
	ar->children = 0;
	ar->type = SHADOW;
	ar->color.w = parent.ray.t_max;
	ar->ray = r;
	ar->owner_id = parent.owner_id;
	ar->tile = parent.tile;
	ar->pixel = parent.pixel;
	return ar;
}
ActiveRay* ActiveRay::secondary(const Ray &r, const ActiveRay &parent) {
	ActiveRay *ar = new ActiveRay(parent);
	ar->children = 0;
	ar->type = SECONDARY;
	ar->color.w = parent.ray.t_max;
	ar->ray = r;
	ar->owner_id = parent.owner_id;
	ar->tile = parent.tile;
	ar->pixel = parent.pixel;
	return ar;
}

}

std::ostream& operator<<(std::ostream &os, const pt::BVHTraversalState &s) {
	os << "BVHTraversalState { current: " << s.current << ", bitstack: "
		<< std::bitset<sizeof(size_t)>(s.bitstack) << " }";
	return os;
}

