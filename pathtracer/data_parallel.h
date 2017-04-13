#pragma once

#include <random>
#include <vector>
#include "pt/pt.h"

class BoundsMessage;

/* A Region is an AABB in the world containing some of the distributed
 * scene data. TODO: It will render the pixels where it's the first hit
 * box and can be sent rays to intersect against its data.
 */
class Region : public CBase_Region {
	// Other regions in the world, along with their bounds
	CProxySection_Region others;
	std::vector<pt::BBox> other_bounds;
	uint64_t bounds_received;

	std::mt19937 rng;

public:
	Region();
	Region(CkMigrateMessage *msg);

	// TODO: Eventually this would load some large
	// mesh file in some distributed fashion
	void load();
	// Receive another region's bounds
	void send_bounds(BoundsMessage *msg);
	// Render the region tiles which this region projects to,
	// compute and send primary rays for any pixels where this Region is the closest box
	void render();
};

class BoundsMessage : public CMessage_BoundsMessage {
public:
	uint64_t id;
	pt::BBox bounds;

	BoundsMessage(const uint64_t id, const pt::BBox &bounds);
	void msg_pup(PUP::er &p);
};

class TileCompleteMessage : public CMessage_TileCompleteMessage {
	// Empty ctor only needed when unpacking a tile message
	TileCompleteMessage();

public:
	// The tile index along x, y, in tile coords
	uint64_t tile_x, tile_y, tile_id;
	// The number of tiles expected from other chares for this
	// final image tile. Non-negative if the chare sending this
	// tile owns the final image tile, otherwise -1.
	int64_t num_other_tiles;
	// Each tile is RGBZF32 data, storing the final pixel color
	// for the pixel in this tile and the Z value of the first hit
	// for compositing.
	std::vector<float> tile;

	// Non-owned tile message constructor
	TileCompleteMessage(const uint64_t tile_x, const uint64_t tile_y);
	// Owned tile message constructor
	TileCompleteMessage(const uint64_t tile_x, const uint64_t tile_y,
			const int64_t num_other_tiles);
	bool tile_owner() const;

	void msg_pup(PUP::er &p);
	static void* pack(TileCompleteMessage *msg);
	static TileCompleteMessage* unpack(void *buf);
};

