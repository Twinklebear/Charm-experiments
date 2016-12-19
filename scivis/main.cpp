#include <ios>
#include <cstring>
#include <cstdio>
#include <string>
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

const static std::string USAGE =
"Arguments:\n"
"\t--vol <file.raw> specify the raw volume file to be rendered\n"
"\t--tile W H       set tile width and height. Default 16 16\n"
"\t--img W H        set image dimensions in tiles. Default 100 100\n"
"\t--spp N          set number of samples taken for each pixel along. Default 1";

Main::Main(CkArgMsg *msg) : spp(1) {
	// Set some default image dimensions
	TILE_W = 16;
	TILE_H = 16;
	// Number of tiles along each dimension
	uint64_t tiles_x = 100;
	uint64_t tiles_y = 100;
	main_proxy = thisProxy;

	std::string volume;
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
			} else if (std::strcmp("--spp", msg->argv[i]) == 0) {
				spp = std::atoi(msg->argv[++i]);
			} else if (std::strcmp("--vol", msg->argv[i]) == 0) {
				volume = msg->argv[++i];
			}
		}
	}
	if (volume.empty()) {
		CkPrintf("Error: A volume file specified with --vol is required.\n%s\n", USAGE.c_str());
	}

	IMAGE_W = TILE_W * tiles_x;
	IMAGE_H = TILE_H * tiles_y;

	CkExit();
}
Main::Main(CkMigrateMessage *msg) {}

#include "main.def.h"

