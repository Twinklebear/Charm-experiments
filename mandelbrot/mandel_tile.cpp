#include <fstream>
#include <unistd.h>
#include "mandel_tile.decl.h"
#include "main.decl.h"
#include "mandel_tile.h"

// readonly global Charm++ variables
extern CProxy_Main main_proxy;
// Image dimensions
extern uint64_t IMAGE_W;
extern uint64_t IMAGE_H;
// Tile x/y dimensions
extern uint64_t TILE_W;
extern uint64_t TILE_H;

MandelTile::MandelTile(uint64_t subsamples) : max_iters(255), subsamples(subsamples) {
	// Compiler on cluster is old POS so gotta fallback to old-style
	x_axis[0] = -1.75f;
	x_axis[1] = 0.75f;
	y_axis[0] = -1.25f;
	y_axis[1] = 1.25f;
}
MandelTile::MandelTile(CkMigrateMessage *msg) {
	CkPrintf("MandelTile #[%d, %d] was migrated\n", thisIndex.x, thisIndex.y);
}

void MandelTile::pup(PUP::er &p) {
	p | x_axis[0];
	p | x_axis[1];
	p | y_axis[0];
	p | y_axis[1];
	p | max_iters;
	p | subsamples;
}
void MandelTile::render() {
	const uint64_t tiles_x = IMAGE_W / TILE_W;
	const uint64_t start_x = thisIndex.x * TILE_W;
	const uint64_t start_y = thisIndex.y * TILE_H;
#if 0
	char hostname[128] = {0};
	gethostname(hostname, 127);
	CkPrintf("MandelTile #[%d, %d] on processor %d starts at [%d, %d], host = %s\n",
			thisIndex.x, thisIndex.y, CkMyPe(), start_x, start_y, hostname);
#endif

	uint8_t *tile = new uint8_t[TILE_W * TILE_H];
	const float dx = (x_axis[1] - x_axis[0]) / IMAGE_W;
	const float dy = (y_axis[1] - y_axis[0]) / IMAGE_H;
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			uint64_t val = 0;
			for (uint64_t sx = 0; sx < subsamples; ++sx) {
				for (uint64_t sy = 0; sy < subsamples; ++sy) {
					const float x = x_axis[0] + (start_x + j) * dx + (sx + 0.5) * dx / subsamples;
					const float y = y_axis[0] + (start_y + i) * dy + (sy + 0.5) * dy / subsamples;
					val += mandel(x, y);
				}
			}
			tile[i * TILE_W + j] = val / (subsamples * subsamples);
		}
	}
	main_proxy.tile_done(start_x, start_y, tile);
	delete[] tile;
}
unsigned int MandelTile::mandel(const float c_real, const float c_imag) {
	unsigned int i = 0;
	float real = c_real;
	float imag = c_imag;
	for (; i < max_iters; ++i){
		if (real * real + imag * imag > 4.f){
			break;
		}
		float next_r = real * real - imag * imag + c_real;
		float next_i = 2.f * real * imag + c_imag;
		real = next_r;
		imag = next_i;
	}
	return i;
}

#include "mandel_tile.def.h"

