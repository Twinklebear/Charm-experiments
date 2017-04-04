#include <memory>
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
Region::Region(CkMigrateMessage *msg) : rng(std::random_device()()), bounds_received(0) {}
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
void Region::send_bounds(BoundsMessage *msg) {
	std::cout << "Got bounds from " << msg->id << " " << msg->bounds << "\n";
	other_bounds[msg->id] = msg->bounds;

	delete msg;
	++bounds_received;
	if (bounds_received == NUM_REGIONS - 1) {
		main_proxy.dbg_region_done();
	}
}

BoundsMessage::BoundsMessage(const uint64_t id, const pt::BBox &bounds) : id(id), bounds(bounds) {}
void BoundsMessage::msg_pup(PUP::er &p) {
	p | id;
	p | bounds;
}

#include "data_parallel.def.h"

