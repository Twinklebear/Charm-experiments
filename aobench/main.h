#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include "aobench_tile.decl.h"

class Main : public CBase_Main {
	uint64_t done_count;
	uint64_t samples_taken;
	uint64_t spp;
	uint64_t num_tiles;
	std::vector<float> image;
	std::chrono::high_resolution_clock::time_point start;
	CProxy_AOBenchTile aobench_tiles;

public:
	Main(CkArgMsg *msg);
	Main(CkMigrateMessage *msg);

	void tile_done(const uint64_t x, const uint64_t y, const float *tile);
};

