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

Region::Region() : rng(std::random_device()()), bounds_received(0) {
	if (thisIndex == 0) {
		std::shared_ptr<pt::BxDF> lambertian_blue = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.1, 0.8));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(0), 1.0, lambertian_blue);
	} else if (thisIndex == 1) {
		std::shared_ptr<pt::BxDF> lambertian_blue = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.1, 0.8));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(1.0, 0.7, 1.0), 0.25, lambertian_blue);
	} else {
		std::shared_ptr<pt::BxDF> lambertian_white = std::make_shared<pt::Lambertian>(glm::vec3(0.8));
		my_object = std::make_shared<pt::Plane>(glm::vec3(0, 0, -2), glm::vec3(0, 0, 1), 4, lambertian_white);
	}
	other_bounds.resize(NUM_REGIONS);
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

	delete msg;
	++bounds_received;
	if (bounds_received == NUM_REGIONS - 1) {
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
			size_t other_regions = 0;
			for (const auto &b : other_screen_bounds) {
				if (touches_tile(start_x, start_y, b)) {
					++other_regions;
				}
			}
			if (touches_tile(start_x, start_y, screen_bounds)) {
				TileCompleteMessage *msg = new TileCompleteMessage(tx, ty, other_regions + 1);
				render_tile(msg->tile, tx * TILE_W, ty * TILE_H);
				main_proxy.tile_done(msg);
			} else if (other_regions == 0 && tid % NUM_REGIONS == thisIndex) {
				// It's our job to fill the tile with background color from the renderer,
				// since no data projects to this tile.
				TileCompleteMessage *msg = new TileCompleteMessage(tx, ty, 1);
				render_tile(msg->tile, tx * TILE_W, ty * TILE_H);
				main_proxy.tile_done(msg);
			}
		}
	}
}
void Region::render_tile(std::vector<float> &tile, const uint64_t start_x, const uint64_t start_y) {
	// TODO: Don't hardcode integrator, camera, read them from scene and keep them around?
	// or at least keep the scene alive, since the Region may have multiple geometry
	const pt::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);
	pt::HitIntegrator integrator(pt::Scene({my_object}, {}));

	std::uniform_real_distribution<float> real_distrib;
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			const float px = j + start_x;
			const float py = i + start_y;
			pt::Ray ray = camera.generate_ray(px, py, {real_distrib(rng), real_distrib(rng)});
			glm::vec3 color = integrator.integrate(ray);
			if (color == glm::vec3(0)) {
				color = glm::vec3(0.1);
			}
			// Tag the tiles based on who owns them
			switch (thisIndex) {
				case 0: color *= glm::vec3(1, 0, 0); break;
				case 1: color *= glm::vec3(0, 1, 0); break;
				case 2: color *= glm::vec3(0, 0, 1); break;
				default: break;
			}

			const size_t tx = (i * TILE_W + j) * 4;
			for (size_t c = 0; c < 3; ++c) {
				tile[tx + c] = color[c];
			}
			tile[tx + 3] = ray.t_max;
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
TileCompleteMessage::TileCompleteMessage(const uint64_t tile_x, const uint64_t tile_y)
	: tile_x(tile_x), tile_y(tile_y), num_other_tiles(-1), tile(TILE_W * TILE_H * 4, 0)
{}
TileCompleteMessage::TileCompleteMessage(const uint64_t tile_x, const uint64_t tile_y,
		const int64_t num_other_tiles)
	: tile_x(tile_x), tile_y(tile_y), num_other_tiles(num_other_tiles), tile(TILE_W * TILE_H * 4, 0)
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

#include "data_parallel.def.h"

