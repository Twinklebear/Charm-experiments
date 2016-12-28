#pragma once

#include <cstdint>

class Main : public CBase_Main {
	uint64_t num_tiles;
	uint64_t done_count;
	std::vector<uint8_t> image;

public:
	Main(CkArgMsg *msg);
	Main(CkMigrateMessage *msg);

	void tile_done(const uint64_t x, const uint64_t y, const uint8_t *tile);
};


