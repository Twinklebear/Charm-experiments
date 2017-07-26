#pragma once

#include <random>
#include <vector>
#include <set>
#include <memory>
#include <unordered_map>
#include "pt/pt.h"

class BoundsMessage;
class IntersectRayMessage;
class ShadeRayMessage;
class RayResultMessage;
struct RayResult;
class TileCompleteMessage;

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
	std::vector<uint64_t> primary_rays_expected;
	std::vector<uint64_t> shadow_rays_expected, shadow_rays_received;
	std::vector<uint64_t> secondary_rays_expected, secondary_rays_received;
	// For debugging, the index of the Charm++ region we're on
	uint64_t charm_index;

	// Construct a new rendering tile, will expect 1 result per pixel by default.
	RenderingTile(const uint64_t tile_x, const uint64_t tile_y, const int64_t num_other_tiles,
			const uint64_t charm_index);
	/* Report a primary ray, informing the tile how many shadow test results to
	 * expect for the pixel to determine completion. If the ray has no children
	 * the result will be written to the tile, result = {R, G, B, Z}
	 */
	void report_primary_ray(const uint64_t px, const uint64_t spawned_shadow_rays,
			const uint64_t spawned_secondary_rays, const glm::vec4 &result);
	/* Report a secondary ray for some pixel in this tile, tells us if it spawned
	 * any shadows rays which we should then expect shading results from.
	 * In the case a secondary ray didn't hit anything (spawned no shadow rays)
	 * we treat it as a shadow and shade with the result color passed.
	 */
	void report_secondary_ray(const uint64_t px, const uint64_t spawned_shadow_rays,
			const uint64_t spawned_secondary_rays, const glm::vec3 &result);
	// Report a shading result for some pixel in this tile
	void report_shadow_ray(const uint64_t px, const glm::vec3 &result);
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
	std::unique_ptr<pt::PathIntegrator> integrator;
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
	// TODO: Description of the ray sending and data stuff.
	void render();
	// Send a ray for traversal through this region's data.
	void intersect_ray(IntersectRayMessage *msg);
	/* Send a ray for shading by this region's data. This region should
	 * be the one who owns the object being shaded
	 */
	void shade_ray(ShadeRayMessage *msg);
	// Report the result of rendering a ray spawned from one of this regions primary rays
	void report_ray(RayResultMessage *msg);

private:
	// Render a tile of the image to the tile passed
	void render_tile(RenderingTile &tile, const uint64_t start_x, const uint64_t start_y,
			const uint64_t tile_id, const std::set<size_t> &regions_in_tile);
	/* Traverse a newly spawned primary/secondary ray through this node's data, potentially
	 * spawning shadow and secondary rays, or simply continuing it on to
	 * another node
	 */
	void start_ray(pt::ActiveRay &ray);
	/* Continue a primary/secondary ray through this node's data, potentially
	 * spawning shadow and secondary rays, or simply continuing it on to
	 * another node
	 */
	void intersect_ray(pt::ActiveRay &ray);
	/* Shade a ray's hit point that hit a local object on this Chare. Will
	 * compute the shaded color, potentially spawning shadow or secondary rays
	 * as needed.
	 */
	void shade_ray(pt::ActiveRay &ray);
	// Report that this ray has hit an object, according to its ray type
	void report_hit(pt::ActiveRay &ray);
	// Report this ray has not hit anything, according to its ray type
	void report_miss(pt::ActiveRay &ray);
	// Report the result of rendering a ray spawned from one of this regions primary rays
	void report_ray(const RayResult &result);
	// Check if this region has data which projects to the tile
	bool touches_tile(const uint64_t start_x, const uint64_t start_y, const pt::BBox &box) const;
	// Project the passed bounding box to the screen
	pt::BBox project_bounds(const pt::BBox &b) const;

	/* TODO: Charm++ doesn't seem to like it when we send messages
	 * to ourself? This is a pretty annoying thing to have to do, but designing
	 * a nice generic dispatcher is also challenging. Maybe by writing this the crappy
	 * way I'll get some idea how I'd like to abstract it
	 */
	void dispatch(const uint64_t to, IntersectRayMessage *msg);
	void dispatch(const uint64_t to, ShadeRayMessage *msg);
	void dispatch(const uint64_t to, RayResultMessage *msg);
};

class BoundsMessage : public CMessage_BoundsMessage {
public:
	uint64_t id;
	pt::BBox bounds;

	BoundsMessage(const uint64_t id, const pt::BBox &bounds);
	void msg_pup(PUP::er &p);
};

class TileCompleteMessage : public CMessage_TileCompleteMessage {
	TileCompleteMessage() = default;

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

/* Message sent to a region that it should intersect the ray
 * with its local data, and continue it if needed
 */
class IntersectRayMessage : public CMessage_IntersectRayMessage {
public:
	// TODO: Packets or larger chunks of rays, compression.
	// Should convert renderer to a stream system and send sorted
	// SoA ray groups which we compress w/ ZFP.
	pt::ActiveRay ray;

	IntersectRayMessage(const pt::ActiveRay &ray);
	void msg_pup(PUP::er &p);
};

/* Message sent to a node that it's the one responsible for shading
 * this ray. The message will be sent to the hit_info.hit_owner Chare
 */
class ShadeRayMessage : public CMessage_ShadeRayMessage {
public:
	pt::ActiveRay ray;

	ShadeRayMessage(const pt::ActiveRay &ray);
	void msg_pup(PUP::er &p);
};

// TODO: This should basically just be sending back the ray, once I clean
// reporting of the spawned rays
struct RayResult {
	// RGBAZ of the rendering result. If Z = INF then there was no hit
	// TODO: Again, packets or larger, compression, etc.
	glm::vec4 result;
	uint64_t tile, pixel;
	pt::RAY_TYPE type;
	// If this ray is a primary ray or "path", this is the # of shadow
	// rays spawned along this path we should expect to get results from
	uint64_t shadow_children, secondary_children;

	RayResult() = default;
	RayResult(const glm::vec4 &result, uint64_t tile, uint64_t pixel,
		pt::RAY_TYPE type, uint64_t shadow_children, uint64_t secondary_children);
};
void operator|(PUP::er &p, RayResult &r);

class RayResultMessage : public CMessage_RayResultMessage {
public:
	RayResult result;

	RayResultMessage(const glm::vec4 &rgbaz, uint64_t tile, uint64_t pixel,
			pt::RAY_TYPE type, uint64_t shadow_children, uint64_t secondary_children);
	void msg_pup(PUP::er &p);
};

