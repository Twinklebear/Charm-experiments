#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <memory>
#include "pt/pt.h"
#include "image_parallel_tile.decl.h"
#include "data_parallel.decl.h"

struct DistributedTile {
	uint64_t tile_x, tile_y;
	// The expected number of tiles we should receive for this region
	int64_t num_tiles;
	// Tile data from each region for compositing to compute the final tile
	std::vector<std::vector<float>> region_tiles;

	DistributedTile();
};

class Main : public CBase_Main {
	uint64_t num_tiles;
	uint64_t done_count;
	uint64_t spp;
	uint64_t samples_taken;
	// Timing stuff
	std::chrono::high_resolution_clock::time_point start_pass, start_render;
	std::vector<float> image;
	// For each tile in the image we store each regions tile for the final image tile
	std::vector<DistributedTile> region_tiles;
	CProxy_ImageParallelTile img_tiles;
	CProxy_Region regions;

public:
	Main(CkArgMsg *msg);
	Main(CkMigrateMessage *msg);

	// Called by each image-parallel rendering or compositing Chare
	// when they've finished computing the tile
	void tile_done(const uint64_t x, const uint64_t y, const float *tile);
	// Called by a Region when it's finished rendering a tile
	void tile_done(TileCompleteMessage *msg);
	void region_loaded();

private:
	// Composite the region tiles from the distributed rendering
	// to the final image
	void composite_image();
};

class SceneMessage : public CMessage_SceneMessage {
public:
	// Info about the camera in the scene
	glm::vec3 cam_pos, cam_target, cam_up;

	SceneMessage(const glm::vec3 &cam_pos, const glm::vec3 &cam_target, const glm::vec3 &cam_up);
	void msg_pup(PUP::er &p);
};

