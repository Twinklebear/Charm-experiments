#include <memory>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>
#include "main.decl.h"
#include "main.h"
#include "data_parallel.decl.h"
#include "data_parallel.h"
#include "pup_operators.h"

// readonly global Charm++ variables
extern CProxy_Main main_proxy;
extern SceneMessage *scene;
extern uint64_t IMAGE_W;
extern uint64_t IMAGE_H;
extern uint64_t TILE_W;
extern uint64_t TILE_H;
extern uint64_t NUM_REGIONS;

RenderingTile::RenderingTile(const uint64_t tile_x, const uint64_t tile_y, const int64_t num_other_tiles)
	: msg(new TileCompleteMessage(tile_x, tile_y, num_other_tiles)), results_expected(TILE_W * TILE_H, 1)
{}
void RenderingTile::report(const uint64_t x, const uint64_t y, const glm::vec4 &result) {
	const size_t tx = x * TILE_W + y;
	report(tx, result);
}
void RenderingTile::report(const uint64_t px, const glm::vec4 &result) {
	const size_t tx = px * 4;
	// TODO: This should turn into an accumulation where we know how many rays we sent
	// for the pixel plus (optionally) the primary ray branch factor and we accumulate
	// until we get all the results back then normalize the color values.
	for (size_t c = 0; c < 4; ++c) {
		msg->tile[tx + c] = result[c];
	}
	if (results_expected[px] == 0) {
		throw std::runtime_error("Unexpected result reported on tile! #" + std::to_string(msg->tile_id));
	}
	results_expected[px] -= 1;
}
bool RenderingTile::complete() const {
	return std::all_of(results_expected.begin(), results_expected.end(), [](const uint64_t &x) { return x == 0; });
}

Region::Region() : rng(std::random_device()()), bounds_received(0) {
	if (thisIndex == 0) {
		std::shared_ptr<pt::BxDF> lambertian_blue = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.1, 0.8));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(0), 1.0, lambertian_blue);
	} else if (thisIndex == 1) {
		std::shared_ptr<pt::BxDF> lambertian_blue = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.1, 0.8));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(1.0, 0.7, 1.0), 0.25, lambertian_blue);
	} else {
		std::shared_ptr<pt::BxDF> lambertian_white = std::make_shared<pt::Lambertian>(glm::vec3(0.8));
		my_object = std::make_shared<pt::Plane>(glm::vec3(0, 0, -2), glm::vec3(0, 0, 1), 2.5, lambertian_white);
	}
	other_bounds.resize(NUM_REGIONS);
	world.resize(NUM_REGIONS);
}
Region::Region(CkMigrateMessage *msg) : rng(std::random_device()()), bounds_received(0) {
	CkPrintf("Region doesn't support migration!\n");
	delete msg;
}
void Region::load() {
	if (NUM_REGIONS == 1) {
		main_proxy.region_loaded();
	} else {
		// Test of computing our bounds and sharing them with others
		// Build up the list of other regions in the world and send them our bounds
		CkVec<CkArrayIndex1D> elems;
		for (size_t i = 0; i < NUM_REGIONS; ++i) {
			if (i != thisIndex) {
				elems.push_back(CkArrayIndex1D(i));
			}
		}

		others = CProxySection_Region::ckNew(thisArrayID, elems.getVec(), elems.size());
		BoundsMessage *msg = new BoundsMessage(thisIndex, my_object->bounds());
		others.send_bounds(msg);
	}
}
void Region::send_bounds(BoundsMessage *msg) {
	other_bounds[msg->id] = msg->bounds;
	world[msg->id] = pt::DistributedRegion(msg->bounds, msg->id);

	delete msg;
	++bounds_received;
	if (bounds_received == NUM_REGIONS - 1) {
		world[thisIndex] = pt::DistributedRegion(my_object->bounds(), thisIndex);
		std::vector<const pt::DistributedRegion*> world_ptrs;
		std::transform(world.begin(), world.end(), std::back_inserter(world_ptrs),
				[](const pt::DistributedRegion &a) { return &a; });
		bvh = pt::BVH(world_ptrs);
		main_proxy.region_loaded();
	}
}
void Region::render() {
	// TODO: Don't hardcode integrator, camera, read them from scene and keep them around?
	// or at least keep the scene alive, since the Region may have multiple geometry
	const pt::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);

	// Project the bounds of this regions data to the screen so we can determine which tiles
	// the region touches, and thus needs to render
	const pt::BBox screen_bounds = project_bounds(my_object->bounds());
	{
		std::stringstream tmp;
		tmp << screen_bounds << ", region bounds = " << my_object->bounds();
		CkPrintf("Region %d screen bounds %s\n", thisIndex, tmp.str().c_str());
	}

	// Project bounds for all other regions so we can find out
	// how many other box project to this tile
	other_screen_bounds.reserve(other_bounds.size());
	for (const auto &b : other_bounds) {
		other_screen_bounds.push_back(project_bounds(b));
	}

	const uint64_t tiles_x = IMAGE_W / TILE_W;
	const uint64_t tiles_y = IMAGE_H / TILE_H;
	for (uint64_t ty = 0; ty < tiles_y; ++ty) {
		for (uint64_t tx = 0; tx < tiles_x; ++tx) {
			// We're the owner of this tile and responsible for telling how many
			// tiles the master should expect. TODO: The tile owner should be
			// the one with the first hit box for the tile, resolved based on Chare
			// index in case of a tie. Also, if no box projects to the tile the
			// owner is determined via this modulo
			const uint64_t tid = ty * tiles_x + tx;
			// TODO: Only render tiles out data touches, one node is responsible
			// for telling how many tiles to expect in total? But how to decide
			// which node without requiring everyone to do the test on all
			// the regions anyway to find out who potentially touches the tile?
			const uint64_t start_x = tx * TILE_W;
			const uint64_t start_y = ty * TILE_H;
			// Count the number of other regions projecting to this tile
			std::set<size_t> regions_in_tile;
			size_t other_regions = 0;
			for (size_t i = 0; i < other_screen_bounds.size(); ++i) {
				if (touches_tile(start_x, start_y, other_screen_bounds[i])) {
					regions_in_tile.insert(i);
					++other_regions;
				}
			}
			// TODO: If I'm not first for any pixel on this tile, I shouldn't send the
			// final tile at all.
			if (touches_tile(start_x, start_y, screen_bounds)) {
				auto it = rendering_tiles.emplace(tid, RenderingTile(tx, ty, other_regions + 1));
				render_tile(it.first->second, tx * TILE_W, ty * TILE_H, regions_in_tile);
				if (it.first->second.complete()) {
					main_proxy.tile_done(it.first->second.msg);
				}
			} else if (other_regions == 0 && tid % NUM_REGIONS == thisIndex) {
				// It's our job to fill the tile with background color from the renderer,
				// since no data projects to this tile.
				RenderingTile tile(tx, ty, 1);
				render_tile(tile, tx * TILE_W, ty * TILE_H, regions_in_tile);
				main_proxy.tile_done(tile.msg);
			}
		}
	}
}
void Region::send_ray(SendRayMessage *msg) {
	// TODO: Don't hardcode integrator, camera, read them from scene and keep them around?
	// or at least keep the scene alive, since the Region may have multiple geometry
	pt::HitIntegrator integrator(pt::Scene({my_object}, {}));
	glm::vec3 color = integrator.integrate(msg->ray);

	if (color != glm::vec3(0)) {
		// Tag the tiles based on who owns them
		switch (thisIndex) {
			case 0: color *= glm::vec3(1, 0, 0); break;
			case 1: color *= glm::vec3(0, 1, 0); break;
			case 2: color *= glm::vec3(0, 0, 1); break;
			default: break;
		}
		thisProxy[msg->owner_id].report_ray(new RayResultMessage(glm::vec4(color, msg->ray.t_max),
					msg->tile, msg->pixel));
	} else {
		// Backtrack in the BVH and ship the ray off to the next region it needs to
		// traverse. If there is no next region send our result back
		const pt::DistributedRegion *next = nullptr;
		if (bvh.backtrack(msg->traversal)) {
			next = bvh.intersect(msg->ray, msg->traversal);
		}
		if (next) {
			thisProxy[next->owner].send_ray(new SendRayMessage(msg->owner_id, msg->tile,
						msg->pixel, msg->ray, msg->traversal));
		} else {
			color = glm::vec3(0.1);
			switch (thisIndex) {
				case 0: color *= glm::vec3(1, 0, 0); break;
				case 1: color *= glm::vec3(0, 1, 0); break;
				case 2: color *= glm::vec3(0, 0, 1); break;
				default: break;
			}
			thisProxy[msg->owner_id].report_ray(new RayResultMessage(glm::vec4(color, msg->ray.t_max),
						msg->tile, msg->pixel));
		}
	}
	delete msg;
}
void Region::report_ray(RayResultMessage *msg) {
	auto rt = rendering_tiles.find(msg->tile);
	if (rt == rendering_tiles.end()) {
		throw std::runtime_error("Invalid msg->tile id in RayResultMessage!");
	}
	rt->second.report(msg->pixel, msg->result);
	delete msg;

	if (rt->second.complete()) {
		main_proxy.tile_done(rt->second.msg);
	}
}
void Region::render_tile(RenderingTile &tile, const uint64_t start_x, const uint64_t start_y,
		const std::set<size_t> &regions_in_tile)
{
	// TODO: Don't hardcode integrator, camera, read them from scene and keep them around?
	// or at least keep the scene alive, since the Region may have multiple geometry
	const pt::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);
	pt::HitIntegrator integrator(pt::Scene({my_object}, {}));

	// We use a separate rng for primary ray directions so
	// that all regions will see the same ray directions for
	// each tile when testing who is first. TODO: in future send
	// a set of random seeds to rotate through or something to have
	// it not be totally deterministic.
	std::mt19937 ray_dir_rng(start_x + start_y * IMAGE_W);
	std::uniform_real_distribution<float> real_distrib;
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			const float px = j + start_x;
			const float py = i + start_y;
			pt::Ray ray = camera.generate_ray(px, py, {real_distrib(ray_dir_rng), real_distrib(ray_dir_rng)});

			pt::BVHTraversalState traversal;
			const pt::DistributedRegion *first_region = bvh.intersect(ray, traversal);

			glm::vec3 color(0);
			if (first_region && first_region->owner == thisIndex) {
				// If we didn't hit anything, find the next region along the ray and send
				// the ray to it for rendering
				color = integrator.integrate(ray);
				if (color == glm::vec3(0)) {
					// Backtrack in the BVH and ship the ray off to the next region it needs to
					// traverse. If there is no next region, write the background color
					const pt::DistributedRegion *next = nullptr;
					if (bvh.backtrack(traversal)) {
						next = bvh.intersect(ray, traversal);
					}
					if (next) {
						thisProxy[next->owner].send_ray(new SendRayMessage(thisIndex, tile.msg->tile_id,
									i * TILE_W + j, ray, traversal));
					} else {
						color = glm::vec3(0.1);
					}
				}
			} else {
				color = glm::vec3(0.1);
			}
			if (color != glm::vec3(0)) {
				// Tag the tiles based on who owns them
				switch (thisIndex) {
					case 0: color *= glm::vec3(1, 0, 0); break;
					case 1: color *= glm::vec3(0, 1, 0); break;
					case 2: color *= glm::vec3(0, 0, 1); break;
					default: break;
				}
				tile.report(i, j, glm::vec4(color, ray.t_max));
			}
		}
	}
}
bool Region::touches_tile(const uint64_t start_x, const uint64_t start_y, const pt::BBox &bounds) const {
	const pt::BBox tile_bounds(glm::vec3(start_x, start_y, 0.0),
			glm::vec3(start_x + TILE_W, start_y + TILE_H, 0.0));
	return bounds.overlaps(tile_bounds);
}
pt::BBox Region::project_bounds(const pt::BBox &b) const {
	// TODO: Don't hardcode integrator, camera, read them from scene and keep them around?
	// or at least keep the scene alive, since the Region may have multiple geometry
	const pt::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);

	pt::BBox screen_bounds;
	// Compute projection of the region's bounds
	for (size_t i = 0; i < 2; ++i) {
		for (size_t j = 0; j < 2; ++j) {
			for (size_t k = 0; k < 2; ++k) {
				const glm::vec3 d = glm::normalize(glm::vec3(b[k].x, b[j].y, b[i].z) - camera.eye_pos());
				const glm::vec2 p = camera.project_ray(d);
				screen_bounds.extend(glm::vec3(glm::ceil(p), 0.0));
				screen_bounds.extend(glm::vec3(glm::floor(p), 0.0));
			}
		}
	}
	return screen_bounds;
}

BoundsMessage::BoundsMessage(const uint64_t id, const pt::BBox &bounds) : id(id), bounds(bounds) {}
void BoundsMessage::msg_pup(PUP::er &p) {
	p | id;
	p | bounds;
}

TileCompleteMessage::TileCompleteMessage() {}
TileCompleteMessage::TileCompleteMessage(const uint64_t tile_x, const uint64_t tile_y,
		const int64_t num_other_tiles)
	: tile_x(tile_x), tile_y(tile_y), tile_id(tile_x + IMAGE_W / TILE_W * tile_y),
	num_other_tiles(num_other_tiles), tile(TILE_W * TILE_H * 4, 0)
{}
bool TileCompleteMessage::tile_owner() const {
	return num_other_tiles > -1;
}
void TileCompleteMessage::msg_pup(PUP::er &p) {
	p | tile_x;
	p | tile_y;
	p | num_other_tiles;
	p | tile;
}
void* TileCompleteMessage::pack(TileCompleteMessage *msg) {
	PUP::sizer sizer;
	msg->msg_pup(sizer);

	void *buf = CkAllocBuffer(msg, sizer.size());

	PUP::toMem to_mem(buf);
	msg->msg_pup(to_mem);
	delete msg;
	return buf;
}
TileCompleteMessage* TileCompleteMessage::unpack(void *buf) {
	void *msg_buf = CkAllocBuffer(buf, sizeof(TileCompleteMessage));
	TileCompleteMessage *msg = new (msg_buf) TileCompleteMessage();
	PUP::fromMem from_mem(buf);
	msg->msg_pup(from_mem);
	CkFreeMsg(buf);
	return msg;
}

SendRayMessage::SendRayMessage() : ray(glm::vec3(NAN), glm::vec3(NAN)) {}
SendRayMessage::SendRayMessage(uint64_t owner_id, uint64_t tile, uint64_t pixel,
		const pt::Ray &ray, pt::BVHTraversalState traversal)
	: owner_id(owner_id), tile(tile), pixel(pixel), ray(ray), traversal(traversal)
{}
void SendRayMessage::msg_pup(PUP::er &p) {
	p | owner_id;
	p | tile;
	p | pixel;
	p | ray;
	p | traversal.current;
	p | traversal.bitstack;
}

RayResultMessage::RayResultMessage() {}
RayResultMessage::RayResultMessage(const glm::vec4 &result, uint64_t tile, uint64_t pixel)
	: result(result), tile(tile), pixel(pixel)
{}
void RayResultMessage::msg_pup(PUP::er &p) {
	p | result;
	p | tile;
	p | pixel;
}

#include "data_parallel.def.h"

