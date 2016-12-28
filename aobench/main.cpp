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

Main::Main(CkArgMsg *msg) : done_count(0), samples_taken(0), spp(1) {
	// Set some default image dimensions
	TILE_W = 16;
	TILE_H = 16;
	// Number of tiles along each dimension
	uint64_t tiles_x = 100;
	uint64_t tiles_y = 100;
	main_proxy = thisProxy;
	uint64_t ao_samples = 4;

	if (msg->argc > 1) {
		for (int i = 1; i < msg->argc; ++i) {
			if (std::strcmp("-h", msg->argv[i]) == 0){
				CkPrintf("Arguments:\n"
						"\t--tile W H       set tile width and height. Default 16 16\n"
						"\t--img W H        set image dimensions in tiles. Default 100 100\n"
						"\t--spp N          set number of samples taken for each pixel along. Default 1\n"
						"\t--ao-samples N   set the number of AO samples taken. Default 4\n");
				CkExit();
			} else if (std::strcmp("--tile", msg->argv[i]) == 0) {
				TILE_W = std::atoi(msg->argv[++i]);
				TILE_H = std::atoi(msg->argv[++i]);
			} else if (std::strcmp("--img", msg->argv[i]) == 0) {
				tiles_x = std::atoi(msg->argv[++i]);
				tiles_y = std::atoi(msg->argv[++i]);
			} else if (std::strcmp("--spp", msg->argv[i]) == 0) {
				spp = std::atoi(msg->argv[++i]);
			} else if (std::strcmp("--ao-samples", msg->argv[i]) == 0) {
				ao_samples = std::atoi(msg->argv[++i]);
			}
		}
	}
	IMAGE_W = TILE_W * tiles_x;
	IMAGE_H = TILE_H * tiles_y;
	num_tiles = tiles_x * tiles_y;
	image.resize(IMAGE_W * IMAGE_H, 0);

	CkPrintf("Rendering %dx%d AOBench with %dx%d tile size\n"
			"\tSampling: %d samples/pixel, %d AO samples\n",
			IMAGE_W, IMAGE_H, TILE_W, TILE_H, spp, ao_samples);

	aobench_tiles = CProxy_AOBenchTile::ckNew(ao_samples, tiles_x, tiles_y);
	start_pass = start_render = std::chrono::high_resolution_clock::now();
	aobench_tiles.render_sample();
}
Main::Main(CkMigrateMessage *msg) {
	CkPrintf("Main chare is migrating!?\n");
}

// TODO: The Charm++ runtime deletes the tile array for me? If I delete[] it
// I get a double-free error.
void Main::tile_done(const uint64_t x, const uint64_t y, const float *tile) {
	//CkPrintf("Main got tile [%d, %d]\n", x, y);
	// Write this tiles data into the image
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			image[(i + y) * IMAGE_W + j + x] += tile[i * TILE_W + j];
		}
	}
	++done_count;

	// Check if we've finished rendering all the tiles and can start the next set of samples
	if (done_count == num_tiles) {
		++samples_taken;
		done_count = 0;
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_pass);
		CkPrintf("Iteration took %dms\n", duration.count());
		if (samples_taken == spp) {
			duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_render);
			CkPrintf("Rendering took %dms\n", duration.count());
			std::vector<char> image_out(IMAGE_W * IMAGE_H, 0);
			for (uint64_t i = 0; i < IMAGE_H; ++i) {
				for (uint64_t j = 0; j < IMAGE_W; ++j) {
					image_out[i * IMAGE_W + j] = (image[i * IMAGE_W + j] / spp) * 255.0;
				}
			}
			// It seems that CkExit doesn't finish destructors? The ofstream sometimes
			// won't flush and write to disk if I don't have its dtor called before CkExit
			{
				std::ofstream fout("charm_aobench.pgm");
				fout << "P5\n" << IMAGE_W << " " << IMAGE_H << "\n255\n";
				fout.write(image_out.data(), IMAGE_W * IMAGE_H);
				fout << "\n";
			}
			CkExit();
		} else {
			CkPrintf("Starting next iteration of rendering\n");
			start_pass = std::chrono::high_resolution_clock::now();
			aobench_tiles.render_sample();
		}
	}
}

#include "main.def.h"

