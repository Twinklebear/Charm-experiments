#include <memory>
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

Region::Region() : rng(std::random_device()()), bounds_received(0) {}
Region::Region(CkMigrateMessage *msg) : rng(std::random_device()()), bounds_received(0) {
	delete msg;
}
void Region::load() {
	std::shared_ptr<pt::Sphere> my_object = nullptr;
	if (thisIndex == 0) {
		std::shared_ptr<pt::BxDF> lambertian_blue = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.1, 0.8));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(0), 1.0, lambertian_blue);
	} else if (thisIndex == 1) {
		std::shared_ptr<pt::BxDF> lambertian_blue = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.1, 0.8));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(1.0, 0.7, 1.0), 0.25, lambertian_blue);
	} else {
		std::shared_ptr<pt::BxDF> lambertian_red = std::make_shared<pt::Lambertian>(glm::vec3(0.8, 0.1, 0.1));
		my_object = std::make_shared<pt::Sphere>(glm::vec3(-1, -0.75, 1.2), 0.5, lambertian_red);
	}

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
		other_bounds.resize(NUM_REGIONS);
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
	const pt::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);

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
			if (tid % NUM_REGIONS == thisIndex) {
				TileCompleteMessage *msg = new TileCompleteMessage(tx, ty, 1);
				std::fill(msg->tile.begin(), msg->tile.end(), static_cast<float>(thisIndex) / NUM_REGIONS);
				main_proxy.tile_done(msg);
			}
		}
	}
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

