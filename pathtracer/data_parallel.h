#pragma once

#include <random>
#include <vector>
#include <set>
#include <memory>
#include <unordered_map>
#include "pt/pt.h"

class BoundsMessage;

/* A tile we own which is being actively rendered. We could
 * be the only region projecting to the tile and our objects
 * be hit by each ray and thus will trivially render it locally,
 * or it could be that we need to ship some primary rays or secondary
 * rays out to other regions and track how many results back we expect
 * for each pixel before this tile can be marked done.
 */
struct RenderingTile {
	// The partial tile complete message we're rendering in to
	TileCompleteMessage *tile;
	// Count of how many results we're expecting back for each pixel in the tile.
	// Once all entries are 0, this tile is finished.
	std::vector<uint64_t> results_expected;

	// Construct a new rendering tile, will expect 1 result per pixel by default.
	RenderingTile(const uint64_t tile_x, const uint64_t tile_y, const int64_t num_other_tiles);
	// Report a rendering result for some pixel in this tile, result = {R, G, B, Z}
	void report(const uint64_t x, const uint64_t y, const glm::vec4 &result);
	bool complete() const;
};

/* A Region is an AABB in the world containing some of the distributed
 * scene data. TODO: It will render the pixels where it's the first hit
 * box and can be sent rays to intersect against its data.
 */
class Region : public CBase_Region {
	// Other regions in the world, along with their bounds
	CProxySection_Region others;
	// TODO: We need to serialize or save/reload the object if
	// the chare migrates
	std::shared_ptr<pt::Geometry> my_object;
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
	void render_tile(std::vector<float> &tile, const uint64_t start_x, const uint64_t start_y,
			const std::set<size_t> &regions_in_tile);
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

	TileCompleteMessage(const uint64_t tile_x, const uint64_t tile_y,
			const int64_t num_other_tiles);
	bool tile_owner() const;

	void msg_pup(PUP::er &p);
	static void* pack(TileCompleteMessage *msg);
	static TileCompleteMessage* unpack(void *buf);
};

