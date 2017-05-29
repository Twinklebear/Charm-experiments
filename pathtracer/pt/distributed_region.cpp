#include "distributed_region.h"

namespace pt {

DistributedRegion::DistributedRegion() {}
DistributedRegion::DistributedRegion(const BBox &bounds, size_t owner, bool is_mine)
	: bounds(bounds), owner(owner), is_mine(is_mine)
{}

}

