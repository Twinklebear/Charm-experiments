#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include "sv/scivis.h"

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
	// Called when a brick has finished rendering some tile. TODO: currently
	// all bricks render all tiles even if the data doesn't project to the tile.
	// TODO: funneling all tiles to master for compositing is really bad for scaling.
	// TODO: We should send a BrickTileMessage or something so we don't have to
	// make another copy of this tile data.
	void brick_tile_done(const uint64_t tile_x, const uint64_t tile_y, const float *tile);
};

class SceneMessage : public CMessage_SceneMessage {
	SceneMessage();
public:
	// Info about the volume in the scene
	std::string vol_file;
	glm::uvec3 dims, bricking;
	sv::VolumeDType dtype;
	std::shared_ptr<sv::Volume> volume;
	// Info about the camera in the scene
	glm::vec3 cam_pos, cam_target, cam_up;

	SceneMessage(const std::string &vol_file, const glm::uvec3 &dims, const glm::uvec3 &bricking,
			sv::VolumeDType dtype, const glm::vec3 &cam_pos, const glm::vec3 &cam_target,
			const glm::vec3 &cam_up);
	void msg_pup(PUP::er &p);
	bool data_parallel() const;
	uint64_t num_bricks() const;
	static void* pack(SceneMessage *msg);
	static SceneMessage* unpack(void*);
};

