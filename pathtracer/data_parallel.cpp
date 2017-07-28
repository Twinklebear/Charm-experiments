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

RenderingTile::RenderingTile(const uint64_t tile_x, const uint64_t tile_y, const int64_t num_other_tiles,
		const uint64_t charm_index)
	: msg(new TileCompleteMessage(tile_x, tile_y, num_other_tiles)),
	primary_rays_expected(TILE_W * TILE_H, 1),
	shadow_rays_expected(TILE_W * TILE_H, 0),
	shadow_rays_received(TILE_W * TILE_H, 0),
	secondary_rays_expected(TILE_W * TILE_H, 0),
	secondary_rays_received(TILE_W * TILE_H, 0),
	charm_index(charm_index)
{}
void RenderingTile::report_primary_ray(const uint64_t px, const uint64_t spawned_shadow_rays,
		const uint64_t spawned_secondary_rays, const glm::vec4 &result)
{
	if (primary_rays_expected[px] == 0) {
		throw std::runtime_error("Unexpected primary ray reported on tile! #"
				+ std::to_string(msg->tile_id));
	}
	primary_rays_expected[px] -= 1;
	shadow_rays_expected[px] += spawned_shadow_rays;
	secondary_rays_expected[px] += spawned_secondary_rays;

	const size_t tx = px * 4;
	// Primary rays will tell us the depth of the first hit point
	msg->tile[tx + 3] = result.w;
	if (spawned_shadow_rays + spawned_secondary_rays == 0) {
		for (size_t c = 0; c < 3; ++c) {
			msg->tile[tx + c] = result[c];
		}
	}
}
void RenderingTile::report_secondary_ray(const uint64_t px, const uint64_t spawned_shadow_rays,
		const uint64_t spawned_secondary_rays, const glm::vec3 &result) {
	secondary_rays_received[px] += 1;
	secondary_rays_expected[px] += spawned_secondary_rays;
	shadow_rays_expected[px] += spawned_shadow_rays;

	if (spawned_shadow_rays + spawned_secondary_rays == 0) {
		const size_t tx = px * 4;
		for (size_t c = 0; c < 3; ++c) {
			msg->tile[tx + c] += result[c];
		}
	}
}
void RenderingTile::report_shadow_ray(const uint64_t px, const glm::vec3 &result) {
	const size_t tx = px * 4;
	// TODO: This should turn into an accumulation where we know how many rays we sent
	// for the pixel plus (optionally) the primary ray branch factor and we accumulate
	// until we get all the results back then normalize the color values.
	for (size_t c = 0; c < 3; ++c) {
		msg->tile[tx + c] += result[c];
	}
	shadow_rays_received[px] += 1;
}
bool RenderingTile::complete() const {
	const bool primary_done = std::all_of(primary_rays_expected.begin(), primary_rays_expected.end(),
			[](const uint64_t &x) { return x == 0; });
	const bool secondary_done = std::equal(secondary_rays_received.begin(), secondary_rays_received.end(),
			secondary_rays_expected.begin());
	const bool shadow_done = std::equal(shadow_rays_received.begin(), shadow_rays_received.end(),
			shadow_rays_expected.begin());
	return primary_done && shadow_done && secondary_done;
}

Region::Region() : rng(std::random_device()()), bounds_received(0) {
#if 0
	if (thisIndex == 0) {
		std::shared_ptr<pt::BxDF> mat = std::make_shared<pt::Lambertian>(glm::vec3(0.9));
		my_object = std::make_shared<pt::Plane>(glm::vec3(0), glm::vec3(0, 1, 0), 5, mat);
	} else if (thisIndex == 1) {
		std::shared_ptr<pt::BxDF> mat = std::make_shared<pt::Lambertian>(glm::vec3(0.9, 0.3, 0.9));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(0.5, 0.5, 0), 1, mat);
	} else if (thisIndex == 2) {
		std::shared_ptr<pt::BxDF> mat = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.1, 0.8));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(2, 1.5, 0.7), 0.5, mat);
	} else if (thisIndex == 3) {
		std::shared_ptr<pt::BxDF> mat = std::make_shared<pt::Lambertian>(glm::vec3(0.9));
		my_object = std::make_shared<pt::Plane>(glm::vec3(0, 0, -3), glm::vec3(0, 0, 1), 5, mat);
	} else if (thisIndex == 4) {
		std::shared_ptr<pt::BxDF> mat = std::make_shared<pt::Lambertian>(glm::vec3(0.25, 0.75, 0.25));
		my_object = std::make_shared<pt::Plane>(glm::vec3(3, 0, 0), glm::vec3(-1, 0, 0), 5, mat);
	} else if (thisIndex == 5) {
		std::shared_ptr<pt::BxDF> mat = std::make_shared<pt::Lambertian>(glm::vec3(0.75, 0.25, 0.25));
		my_object = std::make_shared<pt::Plane>(glm::vec3(-3, 0, 0), glm::vec3(1, 0, 0), 5, mat);
	} else if (thisIndex == 6) {
		std::shared_ptr<pt::BxDF> mat = std::make_shared<pt::Lambertian>(glm::vec3(0.9));
		my_object = std::make_shared<pt::Plane>(glm::vec3(0, 4, 0), glm::vec3(0, -1, 0), 4.5, mat);
	} else if (thisIndex == 7) {
		std::shared_ptr<pt::BxDF> mat = std::make_shared<pt::Lambertian>(glm::vec3(0.3, 0.9, 0.9));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(-1.5, 1, -0.2), 0.7, mat);
	} else if (thisIndex == 8) {
		std::shared_ptr<pt::BxDF> mat = std::make_shared<pt::Lambertian>(glm::vec3(0.75, 0.25, 0.45));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(-1, 0, 1), 0.5, mat);
	} else {
		throw std::runtime_error("too many test regions!");
	}
#else
	std::shared_ptr<pt::BxDF> lambertian_blue = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.1, 0.8));
	std::shared_ptr<pt::BxDF> lambertian_white = std::make_shared<pt::Lambertian>(glm::vec3(0.8));
	std::shared_ptr<pt::BxDF> lambertian_red = std::make_shared<pt::Lambertian>(glm::vec3(0.8, 0.1, 0.1));
	std::shared_ptr<pt::BxDF> reflective = std::make_shared<pt::SpecularReflection>(glm::vec3(0.8));
	std::vector<std::shared_ptr<pt::Geometry>> objs = {
		std::make_shared<pt::Sphere>(glm::vec3(0), 1.0, lambertian_blue),
		std::make_shared<pt::Sphere>(glm::vec3(1.0, 0.7, 1.0), 0.25, lambertian_blue),
		std::make_shared<pt::Sphere>(glm::vec3(-1, -0.75, 1.2), 0.5, lambertian_red),
		std::make_shared<pt::Plane>(glm::vec3(0, -1, 0), glm::vec3(0, 1, 0), 4, lambertian_white),
		std::make_shared<pt::Plane>(glm::vec3(0, 2, 0), glm::vec3(0, -1, 0), 4, lambertian_white),
		std::make_shared<pt::Plane>(glm::vec3(-1.5, 0, 0), glm::vec3(1, 0, 0), 4, lambertian_white),
		std::make_shared<pt::Plane>(glm::vec3(1.5, 0, 0), glm::vec3(-1, 0, 0), 4, lambertian_white),
		std::make_shared<pt::Plane>(glm::vec3(0, 0, -2), glm::vec3(0, 0, 1), 4, lambertian_white)
	};
	if (thisIndex >= objs.size()) {
		throw std::runtime_error("Too many test regions!");
	}
	my_object = objs[thisIndex];
#endif

	other_bounds.resize(NUM_REGIONS);
	world.resize(NUM_REGIONS);
}
Region::Region(CkMigrateMessage *msg) : rng(std::random_device()()), bounds_received(0) {
	delete msg;
	throw std::runtime_error("Region doesn't support migration!");
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

		// TODO: Don't hardcode integrator, camera, read them from scene and keep them around?
		// or at least keep the scene alive, since the Region may have multiple geometry
		integrator = std::unique_ptr<pt::PathIntegrator>(new pt::PathIntegrator(glm::vec3(0.05),
			pt::Scene({my_object},
			{
			//	std::make_shared<pt::PointLight>(glm::vec3(-0.5, 2, 1), glm::vec3(2)),
				std::make_shared<pt::PointLight>(glm::vec3(0, 1.5, 0.5), glm::vec3(0.9)),
			},
			&bvh
		)));
		main_proxy.region_loaded();

		// TODO: We should save these bounds
		const pt::BBox screen_bounds = project_bounds(my_object->bounds());
		{
			std::stringstream tmp;
			tmp << screen_bounds << ", region bounds = " << my_object->bounds();
			CkPrintf("Region %d screen bounds %s\n", thisIndex, tmp.str().c_str());
		}
		// Project bounds for all other regions so we can find out
		// how many other box project to tiles in the image
		other_screen_bounds.reserve(other_bounds.size());
		for (const auto &b : other_bounds) {
			other_screen_bounds.push_back(project_bounds(b));
		}
	}
}
void Region::render() {
	rendering_tiles.clear();

	// TODO: Don't hardcode integrator, camera, read them from scene and keep them around?
	// or at least keep the scene alive, since the Region may have multiple geometry
	const pt::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);

	// TODO: We should save these bounds
	const pt::BBox screen_bounds = project_bounds(my_object->bounds());

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
				if (it.first->second.msg && it.first->second.complete()) {
					main_proxy.tile_done(it.first->second.msg);
					it.first->second.msg = nullptr;
				}
			} else if (other_regions == 0 && tid % NUM_REGIONS == thisIndex) {
				// It's our job to fill the tile with background color from the renderer,
				// since no data projects to this tile.
				auto it = rendering_tiles.emplace(tid, RenderingTile(tx, ty, 1, thisIndex));
				render_tile(it.first->second, tx * TILE_W, ty * TILE_H, tid, regions_in_tile);
				if (it.first->second.msg && it.first->second.complete()) {
					main_proxy.tile_done(it.first->second.msg);
					it.first->second.msg = nullptr;
				}
			}
		}
	}
}
void Region::intersect_ray(IntersectRayMessage *msg) {
	intersect_ray(msg->ray);
	delete msg;
}
void Region::shade_ray(ShadeRayMessage *msg) {
	shade_ray(msg->ray);
	delete msg;
}
void Region::report_ray(RayResultMessage *msg) {
	report_ray(msg->result);
	delete msg;
}
void Region::render_tile(RenderingTile &tile, const uint64_t start_x, const uint64_t start_y,
		const uint64_t tile_id, const std::set<size_t> &regions_in_tile)
{
	// TODO: Don't hardcode integrator, camera, read them from scene and keep them around?
	// or at least keep the scene alive, since the Region may have multiple geometry
	const pt::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);
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
			pt::ActiveRay ray(cam_ray, thisIndex, tile.msg->tile_id, pixel, glm::vec3(1));
			start_ray(ray);
		}
	}
}
void Region::start_ray(pt::ActiveRay &ray) {
	const pt::DistributedRegion *region = bvh.intersect(ray);
	if (region) {
		if (region->is_mine) {
			intersect_ray(ray);
		} else if (ray.type != pt::RAY_TYPE::PRIMARY) {
			// Everyone tries to start the primary rays for pixels their data projects too
			// so don't send primary rays here, the other node has already created it.
			dispatch(region->owner, new IntersectRayMessage(ray));
		} else {
			// We don't own these pixels but still need to mark them completed
			// locally to finish the tile.
			report_miss(ray);
		}
	} else {
		report_miss(ray);
	}
}
void Region::intersect_ray(pt::ActiveRay &ray) {
	const bool hit = integrator->scene.intersect(ray);
	if (hit) {
		ray.hit_info.hit_owner = thisIndex;
	}
	// It's occluded and we're done here
	if (hit && ray.type == pt::RAY_TYPE::SHADOW) {
		report_hit(ray);
	} else {
		const pt::DistributedRegion *next = bvh.continue_intersect(ray);
		if (next) {
			dispatch(next->owner, new IntersectRayMessage(ray));
		} else if (ray.hit_info.hit) {
			report_hit(ray);
		} else {
			report_miss(ray);
		}
	}
}
void Region::shade_ray(pt::ActiveRay &ray) {
	pt::IntersectionResult result = integrator->integrate(ray);
	// If there's no shadow or secondary ray we're shading a back face hit
	if (!result.shadow && !result.secondary) {
		dispatch(ray.owner_id, new RayResultMessage(
					glm::vec4(0, 0, 0, ray.ray.t_max),
					ray.tile, ray.pixel, ray.type, 0, 0));
	} else {
		// Report what we've spawned to the owner
		const uint64_t shadow_child = result.shadow ? 1 : 0;
		const uint64_t secondary_child = result.secondary ? 1 : 0;

		dispatch(ray.owner_id, new RayResultMessage(
					glm::vec4(0, 0, 0, ray.ray.t_max),
					ray.tile, ray.pixel, ray.type,
					shadow_child, secondary_child));
	}

	if (result.secondary) {
		start_ray(*result.secondary);
	}
	if (result.shadow) {
		start_ray(*result.shadow);
	}
}
void Region::report_hit(pt::ActiveRay &ray) {
	if (ray.type == pt::RAY_TYPE::SHADOW) {
		dispatch(ray.owner_id, new RayResultMessage(
					glm::vec4(0, 0, 0, ray.ray.t_max), ray.tile,
					ray.pixel, ray.type, 0, 0));
	} else {
		dispatch(ray.hit_info.hit_owner, new ShadeRayMessage(ray));
	}
}
void Region::report_miss(pt::ActiveRay &ray) {
	if (ray.type != pt::RAY_TYPE::SHADOW) {
		dispatch(ray.owner_id, new RayResultMessage(
					glm::vec4(integrator->background * ray.throughput, ray.ray.t_max),
					ray.tile, ray.pixel, ray.type, 0, 0));
	} else {
		dispatch(ray.owner_id, new RayResultMessage(
					glm::vec4(ray.color, ray.ray.t_max),
					ray.tile, ray.pixel, ray.type, 0, 0));
	}
}
void Region::report_ray(const RayResult &result) {
	auto rt = rendering_tiles.find(result.tile);
	if (rt == rendering_tiles.end()) {
		throw std::runtime_error("Invalid result.tile id in RayResult!");
	}
	switch (result.type) {
		case pt::RAY_TYPE::PRIMARY:
			rt->second.report_primary_ray(result.pixel, result.shadow_children,
					result.secondary_children, result.result);
			break;
		case pt::RAY_TYPE::SECONDARY:
			rt->second.report_secondary_ray(result.pixel, result.shadow_children,
					result.secondary_children, result.result);
			break;
		case pt::RAY_TYPE::SHADOW:
			rt->second.report_shadow_ray(result.pixel, glm::vec3(result.result));
			break;
		default:
			throw std::runtime_error("Unhandled ray type in report_ray!");
	}
	if (rt->second.complete()) {
		main_proxy.tile_done(rt->second.msg);
		rt->second.msg = nullptr;
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
void Region::dispatch(const uint64_t to, IntersectRayMessage *msg) {
	if (thisIndex == to) {
		intersect_ray(msg->ray);
		delete msg;
	} else {
		thisProxy[to].intersect_ray(msg);
	}
}
void Region::dispatch(const uint64_t to, ShadeRayMessage *msg) {
	if (thisIndex == to) {
		shade_ray(msg->ray);
		delete msg;
	} else {
		thisProxy[to].shade_ray(msg);
	}
}
void Region::dispatch(const uint64_t to, RayResultMessage *msg) {
	if (thisIndex == to) {
		report_ray(msg->result);
		delete msg;
	} else {
		thisProxy[to].report_ray(msg);
	}
}

BoundsMessage::BoundsMessage(const uint64_t id, const pt::BBox &bounds) : id(id), bounds(bounds) {}
void BoundsMessage::msg_pup(PUP::er &p) {
	p | id;
	p | bounds;
}

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

IntersectRayMessage::IntersectRayMessage(const pt::ActiveRay &ray) : ray(ray) {}
void IntersectRayMessage::msg_pup(PUP::er &p) {
	p | ray;
}

ShadeRayMessage::ShadeRayMessage(const pt::ActiveRay &ray) : ray(ray) {}
void ShadeRayMessage::msg_pup(PUP::er &p) {
	p | ray;
}

RayResult::RayResult(const glm::vec4 &result, uint64_t tile, uint64_t pixel,
		pt::RAY_TYPE type, uint64_t shadow_children, uint64_t secondary_children)
	: result(result), tile(tile), pixel(pixel), type(type), shadow_children(shadow_children),
	secondary_children(secondary_children)
{}
void operator|(PUP::er &p, RayResult &r) {
	p | r.result;
	p | r.tile;
	p | r.pixel;
	int ray_type = r.type;
	p | ray_type;
	r.type = static_cast<pt::RAY_TYPE>(ray_type);
	p | r.shadow_children;
	p | r.secondary_children;
}

RayResultMessage::RayResultMessage(const glm::vec4 &rgbaz, uint64_t tile, uint64_t pixel,
		pt::RAY_TYPE type, uint64_t shadow_children, uint64_t secondary_children)
	: result(rgbaz, tile, pixel, type, shadow_children, secondary_children)
{}
void RayResultMessage::msg_pup(PUP::er &p) {
	p | result;
}

#include "data_parallel.def.h"

