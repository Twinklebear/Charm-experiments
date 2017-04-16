#pragma once

#include <random>
#include <vector>
#include <memory>
#include "pt/pt.h"

class BoundsMessage;

/* A Region is an AABB in the world containing some of the distributed
 * scene data. TODO: It will render the pixels where it's the first hit
 * box and can be sent rays to intersect against its data.
 */
class Region : public CBase_Region {
	// Other regions in the world, along with their bounds
	CProxySection_Region others;
	// TODO: We need to serialize or save/reload the object if
	// the chare migrates
	std::shared_ptr<pt::Sphere> my_object;
	// TODO: We need to serialize this if the chare migrates
	std::vector<pt::BBox> other_bounds, other_screen_bounds;
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

private:
	// Render a tile of the image to the tile passed
	void render_tile(std::vector<float> &tile, const uint64_t start_x, const uint64_t start_y);
	// Check if this region has data which projects to the tile
	bool touches_tile(const uint64_t start_x, const uint64_t start_y, const pt::BBox &box) const;
	// Project the passed bounding box to the screen
	pt::BBox project_bounds(const pt::BBox &b) const;
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

