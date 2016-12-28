#include <iostream>
#include <cstring>
#include <cstdio>
#include <string>
#include <fstream>
#include <memory>

#include "sv/scivis.h"
#include "main.decl.h"
#include "image_parallel_tile.decl.h"
#include "main.h"

// readonly global Charm++ variables
CProxy_Main main_proxy;
// Image dimensions
uint64_t IMAGE_W;
uint64_t IMAGE_H;
// Tile x/y dimensions
uint64_t TILE_W;
uint64_t TILE_H;

const static std::string USAGE =
"Arguments:\n"
"\t--vol <file.raw> X Y Z DTYPE      specify the raw volume file to be rendered\n"
"\t                                      X, Y and Z are the volume dimensions and DTYPE\n"
"\t                                      is the data type, one of:\n"
"\t                                      uint8, uint16, int32, float or double\n"
"\t--tile W H                        set tile width and height. Default 16 16\n"
"\t--img W H                         set image dimensions in tiles. Default 100 100\n";

Main::Main(CkArgMsg *msg) : done_count(0) {
	// Set some default image dimensions
	TILE_W = 32;
	TILE_H = 32;
	// Number of tiles along each dimension
	uint64_t tiles_x = 10;
	uint64_t tiles_y = 10;
	main_proxy = thisProxy;

	std::string volume;
	glm::uvec3 dims;
	sv::VolumeDType dtype;
	if (msg->argc > 1) {
		for (int i = 1; i < msg->argc; ++i) {
			if (std::strcmp("-h", msg->argv[i]) == 0){
				CkPrintf("%s\n", USAGE.c_str());
				CkExit();
			} else if (std::strcmp("--tile", msg->argv[i]) == 0) {
				TILE_W = std::atoi(msg->argv[++i]);
				TILE_H = std::atoi(msg->argv[++i]);
			} else if (std::strcmp("--img", msg->argv[i]) == 0) {
				tiles_x = std::atoi(msg->argv[++i]);
				tiles_y = std::atoi(msg->argv[++i]);
			} else if (std::strcmp("--vol", msg->argv[i]) == 0) {
				volume = msg->argv[++i];
				dims.x = std::atoi(msg->argv[++i]);
				dims.y = std::atoi(msg->argv[++i]);
				dims.z = std::atoi(msg->argv[++i]);
				dtype = sv::parse_volume_dtype(msg->argv[++i]);
			}
		}
	}
	if (volume.empty()) {
		CkPrintf("Error: A volume file specified with --vol is required.\n%s\n", USAGE.c_str());
	}

	IMAGE_W = TILE_W * tiles_x;
	IMAGE_H = TILE_H * tiles_y;
	image.resize(IMAGE_W * IMAGE_H * 3, 0);
	num_tiles = tiles_x * tiles_y;

	CkPrintf("Rendering %dx%d image with %dx%d tile size\n", IMAGE_W, IMAGE_H, TILE_W, TILE_H);

	CProxy_ImageParallelTile img_tiles
		= CProxy_ImageParallelTile::ckNew(volume, dims, dtype, tiles_x, tiles_y);
	img_tiles.render();
}
Main::Main(CkMigrateMessage *msg) {}
void Main::tile_done(const uint64_t x, const uint64_t y, const uint8_t *tile) {
	// Write this tiles data into the image
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			for (uint64_t c = 0; c < 3; ++c) {
				image[((i + y) * IMAGE_W + j + x) * 3 + c] = tile[(i * TILE_W + j) * 3 + c];
			}
		}
	}
	++done_count;
	if (done_count == num_tiles) {
		stbi_write_png("scivis_render.png", IMAGE_W, IMAGE_H, 3, image.data(), IMAGE_W * 3);
		CkExit();
	}
}

#include "main.def.h"

