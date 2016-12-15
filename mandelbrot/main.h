#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include "mandel_tile.decl.h"

class Main : public CBase_Main {
	uint64_t done_count;
	uint64_t iters;
	uint64_t num_tiles;
	std::vector<uint8_t> image;
	std::chrono::high_resolution_clock::time_point start;
	CProxy_MandelTile mandel_tiles;

public:
	Main(CkArgMsg *msg);
	Main(CkMigrateMessage *msg);

	void tile_done(const uint64_t x, const uint64_t y, const uint8_t *tile);
};

