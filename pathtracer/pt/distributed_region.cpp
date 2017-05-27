#include "distributed_region.h"

namespace pt {

DistributedRegion::DistributedRegion() {}
DistributedRegion::DistributedRegion(const BBox &bounds, size_t owner)
	: bounds(bounds), owner(owner)
{}

}

