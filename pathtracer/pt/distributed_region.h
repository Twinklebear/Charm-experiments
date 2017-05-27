#pragma once

#include "bbox.h"

namespace pt {

struct DistributedRegion {
	BBox bounds;
	size_t owner;

	DistributedRegion();
	DistributedRegion(const BBox &bounds, size_t owner);
};

}

