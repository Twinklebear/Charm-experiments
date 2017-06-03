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
	ar->ray = r;
	ar->owner_id = parent.owner_id;
	ar->tile = parent.tile;
	ar->pixel = parent.pixel;
	return ar;
}

}

