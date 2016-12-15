#include <ios>
#include <cstring>
#include <cstdio>
#include <fstream>
#include "main.decl.h"
#include "main.h"

// readonly global Charm++ variables
CProxy_Main main_proxy;
// Image dimensions
uint64_t IMAGE_W;
uint64_t IMAGE_H;
// Tile x/y dimensions
uint64_t TILE_W;
uint64_t TILE_H;

Main::Main(CkArgMsg *msg) : done_count(0), run_iters(1), iters(0) {
	// Set some default image dimensions
	TILE_W = 16;
	TILE_H = 16;
	// Number of tiles along each dimension
	uint64_t tiles_x = 100;
	uint64_t tiles_y = 100;
	main_proxy = thisProxy;
	uint64_t subsamples = 1;

	if (msg->argc > 1) {
		for (int i = 1; i < msg->argc; ++i) {
			if (std::strcmp("-h", msg->argv[i]) == 0){
				CkPrintf("Arguments:\n"
						"\t--tile W H       set tile width and height. Default 16 16\n"
						"\t--img W H        set image dimensions in tiles. Default 100 100\n"
						"\t--samples N      set number of subsamples taken for each pixel along x/y\n"
            "\t                     so the total # of samples will be N^2. Default 1\n");
				CkExit();
			} else if (std::strcmp("--tile", msg->argv[i]) == 0) {
				TILE_W = std::atoi(msg->argv[++i]);
				TILE_H = std::atoi(msg->argv[++i]);
			} else if (std::strcmp("--img", msg->argv[i]) == 0) {
				tiles_x = std::atoi(msg->argv[++i]);
				tiles_y = std::atoi(msg->argv[++i]);
			} else if (std::strcmp("--samples", msg->argv[i]) == 0) {
				subsamples = std::atoi(msg->argv[++i]);
			} else if (std::strcmp("--iters", msg->argv[i]) == 0) {
				run_iters = std::atoi(msg->argv[++i]);
			}
		}
	}
	IMAGE_W = TILE_W * tiles_x;
	IMAGE_H = TILE_H * tiles_y;
	num_tiles = tiles_x * tiles_y;
	image.resize(IMAGE_W * IMAGE_H, 0);

	CkPrintf("Rendering %dx%d Mandelbrot set with %dx%d tile size and %d samples/pixel\n"
			"\tRunning %d iterations for load balancing testing\n",
			IMAGE_W, IMAGE_H, TILE_W, TILE_H, subsamples * subsamples, run_iters);

	mandel_tiles = CProxy_MandelTile::ckNew(subsamples, tiles_x, tiles_y);
	start = std::chrono::high_resolution_clock::now();
	mandel_tiles.render();
}
Main::Main(CkMigrateMessage *msg) {
	CkPrintf("Main chare is migrating!?\n");
}

// TODO: The Charm++ runtime deletes the tile array for me? If I delete[] it
// I get a double-free error.
void Main::tile_done(const uint64_t x, const uint64_t y, const uint8_t *tile) {
	//CkPrintf("Main got tile [%d, %d]\n", x, y);
	// Write this tiles data into the image
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			image[(i + y) * IMAGE_W + j + x] = tile[i * TILE_H + j];
		}
	}
	++done_count;

	// Check if we've finished rendering all the tiles and can
	// save out the final image and exit
	if (done_count == num_tiles) {
		++iters;
		done_count = 0;
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		CkPrintf("Rendering took %dms\n", duration.count());
		if (iters == run_iters) {
			// It seems that CkExit doesn't finish destructors? The ofstream sometimes
			// won't flush and write to disk if I don't have its dtor called before CkExit
			{
				std::ofstream fout("charm_mandel.pgm");
				fout << "P5\n" << IMAGE_W << " " << IMAGE_H << "\n255\n";
				fout.write(reinterpret_cast<const char*>(image.data()), IMAGE_W * IMAGE_H);
				fout << "\n";
			}
			CkExit();
		} else {
			CkPrintf("Starting next iteration of rendering\n");
			start = std::chrono::high_resolution_clock::now();
			mandel_tiles.render();
		}
	}
}

#include "main.def.h"

