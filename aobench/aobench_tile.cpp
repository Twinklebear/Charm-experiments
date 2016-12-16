#include <fstream>
#include <unistd.h>
#include "aobench_tile.decl.h"
#include "main.decl.h"
#include "aobench_tile.h"

// readonly global Charm++ variables
extern CProxy_Main main_proxy;
// Image dimensions
extern uint64_t IMAGE_W;
extern uint64_t IMAGE_H;
// Tile x/y dimensions
extern uint64_t TILE_W;
extern uint64_t TILE_H;

AOBenchTile::AOBenchTile(uint64_t ao_samples) : ao_samples(ao_samples) {}
AOBenchTile::AOBenchTile(CkMigrateMessage *msg) {
	//CkPrintf("AOBenchTile #[%d, %d] was migrated\n", thisIndex.x, thisIndex.y);
}

void AOBenchTile::render_sample() {
	const uint64_t tiles_x = IMAGE_W / TILE_W;
	const uint64_t start_x = thisIndex.x * TILE_W;
	const uint64_t start_y = thisIndex.y * TILE_H;
#if 0
	char hostname[128] = {0};
	gethostname(hostname, 127);
	CkPrintf("AOBenchTile #[%d, %d] on processor %d starts at [%d, %d], host = %s\n",
			thisIndex.x, thisIndex.y, CkMyPe(), start_x, start_y, hostname);
#endif

	float *tile = new float[TILE_W * TILE_H];
	main_proxy.tile_done(start_x, start_y, tile);
	delete[] tile;
}

#include "aobench_tile.def.h"

