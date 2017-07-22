#include <memory>
#include <sstream>
#include <algorithm>
#include <iostream>
#define GLM_FORCE_SWIZZLE
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

RenderingTile::RenderingTile(const uint64_t tile_x, const uint64_t tile_y, const int64_t num_other_tiles,
		const uint64_t charm_index)
	: msg(new TileCompleteMessage(tile_x, tile_y, num_other_tiles)), shadow_rays_expected(TILE_W * TILE_H, 0),
	shadow_rays_received(TILE_W * TILE_H, 0), primary_rays_expected(TILE_W * TILE_H, 1), charm_index(charm_index)
{}
void RenderingTile::report_primary_ray(const uint64_t px, const uint64_t children, const glm::vec4 &result) {
	if (primary_rays_expected[px] == 0) {
		throw std::runtime_error("Unexpected primary ray reported on tile! #"
				+ std::to_string(msg->tile_id));
	}
	primary_rays_expected[px] -= 1;
	shadow_rays_expected[px] += children;
	if (children == 0) {
		const size_t tx = px * 4;
		for (size_t c = 0; c < 4; ++c) {
			msg->tile[tx + c] = result[c];
		}
	}
}
void RenderingTile::report(const uint64_t px, const glm::vec4 &result) {
	const size_t tx = px * 4;
	// TODO: This should turn into an accumulation where we know how many rays we sent
	// for the pixel plus (optionally) the primary ray branch factor and we accumulate
	// until we get all the results back then normalize the color values.
	for (size_t c = 0; c < 4; ++c) {
		msg->tile[tx + c] = result[c];
	}
	shadow_rays_received[px] += 1;
}
bool RenderingTile::complete() const {
	const bool primary_done = *std::max_element(primary_rays_expected.begin(),
			primary_rays_expected.end()) == 0;
	const bool shadow_done = std::equal(shadow_rays_received.begin(), shadow_rays_received.end(),
			shadow_rays_expected.begin());
	return primary_done && shadow_done;
}

Region::Region() : rng(std::random_device()()), bounds_received(0) {
	if (thisIndex == 0) {
		std::shared_ptr<pt::BxDF> lambertian_green = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.8, 0.1));
		my_object = std::make_shared<pt::Plane>(glm::vec3(0), glm::vec3(0, 1, 0.5), 4,
				lambertian_green);
	} else if (thisIndex == 1) {
		std::shared_ptr<pt::BxDF> lambertian_red = std::make_shared<pt::Lambertian>(glm::vec3(0.8, 0.1, 0.1));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(0), 0.7, lambertian_red);
	} else {
		std::shared_ptr<pt::BxDF> lambertian_blue = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.1, 0.8));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(1.0, 0.5, 0.5), 0.25, lambertian_blue);
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
	world[msg->id] = pt::DistributedRegion(msg->bounds, msg->id, false);

	delete msg;
	++bounds_received;
	if (bounds_received == NUM_REGIONS - 1) {
		world[thisIndex] = pt::DistributedRegion(my_object->bounds(), thisIndex, true);
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
	// TODO: project_bounds is very wrong.
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
				regions_in_tile.insert(thisIndex);
				auto it = rendering_tiles.emplace(tid, RenderingTile(tx, ty, other_regions + 1, thisIndex));
				render_tile(it.first->second, tx * TILE_W, ty * TILE_H, tid, regions_in_tile);
				if (it.first->second.complete()) {
					main_proxy.tile_done(it.first->second.msg);
				}
			} else if (other_regions == 0 && tid % NUM_REGIONS == thisIndex) {
				// It's our job to fill the tile with background color from the renderer,
				// since no data projects to this tile.
				RenderingTile tile(tx, ty, 1, thisIndex);
				render_tile(tile, tx * TILE_W, ty * TILE_H, tid, regions_in_tile);
				main_proxy.tile_done(tile.msg);
			}
		}
	}
}
void Region::send_ray(SendRayMessage *msg) {
	// TODO: Don't hardcode integrator, camera, read them from scene and keep them around?
	// or at least keep the scene alive, since the Region may have multiple geometry
	pt::WhittedIntegrator integrator(glm::vec3(0.05),
		pt::Scene({my_object},
			{
				std::make_shared<pt::PointLight>(glm::vec3(1.5, 1.5, 1), glm::vec3(2)),
			},
			&bvh
		));

	/* Reporting rays:
	 * - PRIMARY with no results: We didn't hit anything.
	 *   	1. Send back the background color and 0 kids to tell the owner the ray is done.
	 * - PRIMARY with SHADOW: We hit something
	 *   	1. Send back the primary ray with 1 child count to tell the owner to expect a shadow ray.
	 *   	2. Traverse the shadow ray through the scene and ship it off if needed
	 *
	 * - SHADOW with no hit: The surface point is illuminated.
	 *   	1. Send the owner the shaded result color.
	 * - SHADOW with a hit: The surface point is not illuminated.
	 *   	1. Send the owner back black for the shading result.
	 */
	if (msg->ray.type == pt::RAY_TYPE::PRIMARY) {
		pt::IntersectionResult result = integrator.integrate(msg->ray);
		if (!result.shadow && !result.secondary) {
			// Backtrack in the BVH and ship the ray off to the next region it needs to
			// traverse, if there is no next region send the background color back
			const pt::DistributedRegion *next = nullptr;
			if (bvh.backtrack(msg->ray)) {
				next = bvh.intersect(msg->ray);
			}
			if (next) {
				thisProxy[next->owner].send_ray(new SendRayMessage(msg->ray));
			} else {
				thisProxy[msg->ray.owner_id].report_ray(new RayResultMessage(glm::vec4(integrator.background,
							msg->ray.ray.t_max), msg->ray.tile, msg->ray.pixel,
							msg->ray.type, msg->ray.children));
			}
		} else if (result.shadow) {
			// Report that we've spawned a shadow ray and traverse it
			thisProxy[msg->ray.owner_id].report_ray(new RayResultMessage(glm::vec4(glm::vec3(0),
						msg->ray.ray.t_max), msg->ray.tile, msg->ray.pixel,
						msg->ray.type, 1));

			const pt::DistributedRegion *shadow_region = bvh.intersect(*result.shadow);
			bool occluded = false;
			if (shadow_region && shadow_region->is_mine) {
				occluded = integrator.occluded(*result.shadow);
			}
			// If our local data doesn't occlude the point, is there some other region that might?
			if (!occluded) {
				if (bvh.backtrack(*result.shadow)) {
					shadow_region = bvh.intersect(*result.shadow);
				}
			}
			// If we occluded it with local data, we're done. Otherwise there might be
			// data on some other node which occludes and we need to ship the ray off.
			// If there is no next region to traverse, the point is not occluded.
			if (occluded || result.shadow->color.xyz() == glm::vec3(0)) {
				thisProxy[result.shadow->owner_id].report_ray(new RayResultMessage(
							glm::vec4(glm::vec3(0), result.shadow->color.w),
							result.shadow->tile, result.shadow->pixel,
							pt::RAY_TYPE::SHADOW, 0));
			} else if (shadow_region) {
				thisProxy[shadow_region->owner].send_ray(new SendRayMessage(*result.shadow));
			} else {
				thisProxy[result.shadow->owner_id].report_ray(new RayResultMessage(result.shadow->color,
							result.shadow->tile, result.shadow->pixel, pt::RAY_TYPE::SHADOW, 0));
			}
		}
	} else if (msg->ray.type == pt::RAY_TYPE::SHADOW) {
		bool occluded = integrator.occluded(msg->ray);
		// If our local data doesn't occlude the point, is there some other region that might?
		const pt::DistributedRegion *next = nullptr;
		if (!occluded) {
			if (bvh.backtrack(msg->ray)) {
				next = bvh.intersect(msg->ray);
			}
		}
		// If we occluded it with local data, we're done. Otherwise there might be
		// data on some other node which occludes and we need to ship the ray off.
		// If there is no next region to traverse, the point is not occluded.
		if (occluded) {
			thisProxy[msg->ray.owner_id].report_ray(new RayResultMessage(
						glm::vec4(glm::vec3(0), msg->ray.color.w),
						msg->ray.tile, msg->ray.pixel, msg->ray.type, 0));
		} else if (next) {
			thisProxy[next->owner].send_ray(new SendRayMessage(msg->ray));
		} else {
			thisProxy[msg->ray.owner_id].report_ray(new RayResultMessage(msg->ray.color,
						msg->ray.tile, msg->ray.pixel, msg->ray.type, 0));
		}
	}
	delete msg;
}
void Region::report_ray(RayResultMessage *msg) {
	auto rt = rendering_tiles.find(msg->tile);
	if (rt == rendering_tiles.end()) {
		throw std::runtime_error("Invalid msg->tile id in RayResultMessage!");
	}
	switch (msg->type) {
		case pt::RAY_TYPE::PRIMARY:
			rt->second.report_primary_ray(msg->pixel, msg->children, msg->result);
			break;
		case pt::RAY_TYPE::SHADOW:
			rt->second.report(msg->pixel, msg->result);
			break;
		default:
			std::cout << "Unhandled ray type in report_ray!\n";
	}
	delete msg;

	if (rt->second.complete()) {
		main_proxy.tile_done(rt->second.msg);
	}
}
void Region::render_tile(RenderingTile &tile, const uint64_t start_x, const uint64_t start_y,
		const uint64_t tile_id, const std::set<size_t> &regions_in_tile)
{
	// TODO: Don't hardcode integrator, camera, read them from scene and keep them around?
	// or at least keep the scene alive, since the Region may have multiple geometry
	const pt::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);
	pt::WhittedIntegrator integrator(glm::vec3(0.05),
		pt::Scene({my_object},
			{
				std::make_shared<pt::PointLight>(glm::vec3(1.5, 1.5, 1), glm::vec3(2)),
			},
			&bvh
		));
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
			const uint64_t pixel = i * TILE_W + j;
			const pt::Ray cam_ray = camera.generate_ray(px, py,
					{real_distrib(ray_dir_rng), real_distrib(ray_dir_rng)});
			pt::ActiveRay ray(cam_ray, thisIndex, tile.msg->tile_id, pixel);

			// TODO: How to unify this code-path with the send_ray codepath?
			// I'd like to avoid allocating many ray result messages in the case where
			// we locally render most of the tiles pixels, making the results handling
			// parts just subtly distinct enough to be annoying.
			const pt::DistributedRegion *first_region = bvh.intersect(ray);
			while (first_region && !first_region->is_mine) {
				if (bvh.backtrack(ray)) {
					first_region = bvh.intersect(ray);
				} else {
					first_region = nullptr;
				}
			}
			if (first_region) {
				// If we didn't hit anything, find the next region along the ray and send
				// the ray to it for rendering
				pt::IntersectionResult result = integrator.integrate(ray);
				if (!result.shadow && !result.secondary) {
					// No hit, backtrack in the BVH and ship the ray off to the next region it needs to
					// traverse. If there is no next region, write the background color
					const pt::DistributedRegion *next = nullptr;
					if (bvh.backtrack(ray)) {
						next = bvh.intersect(ray);
					}
					if (next) {
						thisProxy[next->owner].send_ray(new SendRayMessage(ray));
					} else {
						tile.report_primary_ray(pixel, 0, glm::vec4(integrator.background,
									std::numeric_limits<float>::max()));
					}
				} else if (result.shadow && glm::vec3(result.shadow->color) == glm::vec3(0.f)) {
					tile.report_primary_ray(pixel, 0, result.shadow->color);
				} else if (result.shadow) {
					// Report that we've spawned a shadow ray and traverse it
					tile.report_primary_ray(pixel, 1, glm::vec4(glm::vec3(0), ray.ray.t_max));

					const pt::DistributedRegion *shadow_region = bvh.intersect(*result.shadow);
					bool occluded = false;
					if (shadow_region && shadow_region->is_mine) {
						occluded = integrator.occluded(*result.shadow);
					}
					// If our local data doesn't occlude the point, is there some other region that might?
					if (!occluded) {
						if (bvh.backtrack(*result.shadow)) {
							shadow_region = bvh.intersect(*result.shadow);
						}
					}
					// If we occluded it with local data, we're done. Otherwise there might be
					// data on some other node which occludes and we need to ship the ray off.
					// If there is no next region to traverse, the point is not occluded.
					if (occluded || result.shadow->color.xyz() == glm::vec3(0)) {
						tile.report(pixel, glm::vec4(glm::vec3(0), result.shadow->color.w));
					} else if (shadow_region) {
						thisProxy[shadow_region->owner].send_ray(new SendRayMessage(*result.shadow));
					} else {
						tile.report(pixel, result.shadow->color);
					}
				}
			} else {
				// We don't own these pixels but still need to "finish" them on our local tile
				// so we can see it as being completed. TODO: expose background color
				// from the integrator so we don't have a hardcoded 0 background which
				// may not match the scene background
				tile.report_primary_ray(pixel, 0, glm::vec4(integrator.background,
							std::numeric_limits<float>::infinity()));
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

SendRayMessage::SendRayMessage() : ray(pt::Ray(glm::vec3(NAN), glm::vec3(NAN)), 0, 0, 0) {}
SendRayMessage::SendRayMessage(const pt::ActiveRay &ray) : ray(ray) {}
void SendRayMessage::msg_pup(PUP::er &p) {
	int ray_type = ray.type;
	p | ray_type;
	ray.type = static_cast<pt::RAY_TYPE>(ray_type);
	p | ray.ray;
	p | ray.traversal.current;
	p | ray.traversal.bitstack;
	p | ray.color;
	p | ray.owner_id;
	p | ray.tile;
	p | ray.pixel;
}

RayResultMessage::RayResultMessage() {}
RayResultMessage::RayResultMessage(const glm::vec4 &result, uint64_t tile, uint64_t pixel,
		pt::RAY_TYPE type, uint64_t children)
	: result(result), tile(tile), pixel(pixel), type(type), children(children)
{}
void RayResultMessage::msg_pup(PUP::er &p) {
	p | result;
	p | tile;
	p | pixel;
	int ray_type = type;
	p | ray_type;
	type = static_cast<pt::RAY_TYPE>(ray_type);
	p | children;
}

#include "data_parallel.def.h"

