#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <memory>
#include "pt/pt.h"
#include "image_parallel_tile.decl.h"

class Main : public CBase_Main {
	uint64_t num_tiles;
	uint64_t done_count;
	uint64_t spp;
	uint64_t samples_taken;
	// Timing stuff
	std::chrono::high_resolution_clock::time_point start_pass, start_render;
	std::vector<float> image;
	// For each tile in the image we store all tiles sent for it
	// by the different bricks. brick_tiles [ tile 0: [brick 0, brick 1], tile 1...]
	std::vector<std::vector<std::vector<float>>> brick_tiles;
	CProxy_ImageParallelTile img_tiles;

public:
	Main(CkArgMsg *msg);
	Main(CkMigrateMessage *msg);

	// Called by each image-parallel rendering or compositing Chare
	// when they've finished computing the tile
	void tile_done(const uint64_t x, const uint64_t y, const float *tile);
	void dbg_region_done();
};

class SceneMessage : public CMessage_SceneMessage {
	SceneMessage();

public:
	// Info about the camera in the scene
	glm::vec3 cam_pos, cam_target, cam_up;

	SceneMessage(const glm::vec3 &cam_pos, const glm::vec3 &cam_target, const glm::vec3 &cam_up);
	void msg_pup(PUP::er &p);
	// Note: note needed for simple fixed-size msg, but keeping it around
	// for reference.
	//static void* pack(SceneMessage *msg);
	//static SceneMessage* unpack(void*);
};

