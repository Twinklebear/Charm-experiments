#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include "pt/pt.h"

class Main : public CBase_Main {
	uint64_t num_tiles;
	uint64_t done_count;
	std::vector<uint8_t> image;
	// For each tile in the image we store all tiles sent for it
	// by the different bricks. brick_tiles [ tile 0: [brick 0, brick 1], tile 1...]
	std::vector<std::vector<std::vector<float>>> brick_tiles;

public:
	Main(CkArgMsg *msg);
	Main(CkMigrateMessage *msg);

	// Called by each image-parallel rendering or compositing Chare
	// when they've finished computing the tile
	void tile_done(const uint64_t x, const uint64_t y, const uint8_t *tile);
};

class SceneMessage : public CMessage_SceneMessage {
	SceneMessage();

public:
	// Info about the camera in the scene
	glm::vec3 cam_pos, cam_target, cam_up;

	SceneMessage(const glm::vec3 &cam_pos, const glm::vec3 &cam_target, const glm::vec3 &cam_up);
	void msg_pup(PUP::er &p);
	static void* pack(SceneMessage *msg);
	static SceneMessage* unpack(void*);
};

