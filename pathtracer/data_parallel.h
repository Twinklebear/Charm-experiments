#pragma once

#include <random>
#include <vector>
#include <set>
#include <memory>
#include <unordered_map>
#include "pt/pt.h"

class BoundsMessage;
class SendRayMessage;
class RayResultMessage;

/* A tile we own which is being actively rendered. We could
 * be the only region projecting to the tile and our objects
 * be hit by each ray and thus will trivially render it locally,
 * or it could be that we need to ship some primary rays or secondary
 * rays out to other regions and track how many results back we expect
 * for each pixel before this tile can be marked done.
 *
 * While the RenderingTile allocates the tile complete message it will
 * not de-allocate it. It's expected that when it's finished you will send
 * the message off thus passing ownership off to the messaging system.
 */
struct RenderingTile {
	// The partial tile complete message we're rendering in to
	TileCompleteMessage *msg;
	// Count of how many results we're expecting back for each pixel in the tile.
	// Once all entries are 0, this tile is finished.
	std::vector<uint64_t> results_expected;
	// For debugging, the index of the Charm++ region we're on
	uint64_t charm_index;

	// Construct a new rendering tile, will expect 1 result per pixel by default.
	RenderingTile(const uint64_t tile_x, const uint64_t tile_y, const int64_t num_other_tiles,
			const uint64_t charm_index);
	// Report a rendering result for some pixel in this tile, result = {R, G, B, Z}
	void report(const uint64_t x, const uint64_t y, const glm::vec4 &result);
	// Report a rendering result for some pixel in this tile, result = {R, G, B, Z}
	void report(const uint64_t px, const glm::vec4 &result);
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
	std::vector<pt::DistributedRegion> world;
	pt::BVH bvh;
	// Tile's we're actively rendering or waiting for results back from
	// other nodes to complete rendering.
	std::unordered_map<size_t, RenderingTile> rendering_tiles;
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
	// Send a ray for traversal through this region's data.
	void send_ray(SendRayMessage *msg);
	// Report the result of rendering a ray spawned from one of this regions primary rays
	void report_ray(RayResultMessage *msg);

private:
	// Render a tile of the image to the tile passed
	void render_tile(RenderingTile &tile, const uint64_t start_x, const uint64_t start_y,
			const uint64_t tile_id, const std::set<size_t> &regions_in_tile);
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

class SendRayMessage : public CMessage_SendRayMessage {
	SendRayMessage();

public:
	uint64_t owner_id, tile, pixel;
	// TODO: Packets or larger chunks of rays, compression.
	// Should convert renderer to a stream system and send sorted
	// SoA ray groups which we compress w/ ZFP.
	pt::Ray ray;
	pt::BVHTraversalState traversal;

	SendRayMessage(uint64_t owner_id, uint64_t tile, uint64_t pixel, const pt::Ray &ray,
			pt::BVHTraversalState traversal);
	void msg_pup(PUP::er &p);
};

class ShadowRayMessage : public CMessage_ShadowRayMessage {
	ShadowRayMessage();

public:
	uint64_t owner_id, tile, pixel;
	// Partial shading is the color we're accumulating into for this pixel,
	// while surface color is the current surface being shaded, to be multiplied
	// with the light color if it's not occluded.
	glm::vec3 partial_shading, surface_color;
	// TODO: Packets or larger chunks of rays, compression.
	// Should convert renderer to a stream system and send sorted
	// SoA ray groups which we compress w/ ZFP.
	pt::Ray ray;
	pt::BVHTraversalState traversal;
};

class RayResultMessage : public CMessage_RayResultMessage {
	RayResultMessage();

public:
	// RGBAZ of the rendering result. If Z = INF then there was no hit
	// TODO: Again, packets or larger, compression, etc.
	glm::vec4 result;
	uint64_t tile, pixel;

	RayResultMessage(const glm::vec4 &result, uint64_t tile, uint64_t pixel);
	void msg_pup(PUP::er &p);
};

