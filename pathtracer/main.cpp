#include <iostream>
#include <utility>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <string>
#include <fstream>
#include <memory>

#include <pup.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "pt/pt.h"
#include "main.decl.h"
#include "image_parallel_tile.decl.h"
#include "data_parallel.decl.h"
#include "data_parallel.h"
#include "main.h"
#include "pup_operators.h"

// readonly global Charm++ variables
CProxy_Main main_proxy;
SceneMessage *scene;
// Image dimensions
uint64_t IMAGE_W;
uint64_t IMAGE_H;
// Tile x/y dimensions
uint64_t TILE_W;
uint64_t TILE_H;
uint64_t NUM_REGIONS;

const static std::string USAGE =
"Arguments:\n"
"\t--tile W H    set tile width and height. Default 16 16\n"
"\t--img W H     set image dimensions in tiles. Default 100 100\n";

float linear_to_srgb(const float x) {
	const float a = 0.055;
	const float b = 1.f / 2.4f;
	if (x <= 0.0031308) {
		return 12.92 * x;
	} else {
		return (1.0 + a) * std::pow(x, b) - a;
	}
}

DistributedTile::DistributedTile() : tile_x(0), tile_y(0), num_tiles(-1) {}

Main::Main(CkArgMsg *msg) : done_count(0), spp(1), samples_taken(0) {
	// Set some default image dimensions
	TILE_W = 32;
	TILE_H = 32;
	// Number of tiles along each dimension
	uint64_t tiles_x = 10;
	uint64_t tiles_y = 10;
	main_proxy = thisProxy;

	glm::vec3 cam_pos(0), cam_target(0), cam_up(0, 1, 0);
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
			} else if (std::strcmp("--pos", msg->argv[i]) == 0) {
				cam_pos.x = std::atof(msg->argv[++i]);
				cam_pos.y = std::atof(msg->argv[++i]);
				cam_pos.z = std::atof(msg->argv[++i]);
			} else if (std::strcmp("--target", msg->argv[i]) == 0) {
				cam_target.x = std::atof(msg->argv[++i]);
				cam_target.y = std::atof(msg->argv[++i]);
				cam_target.z = std::atof(msg->argv[++i]);
			} else if (std::strcmp("--up", msg->argv[i]) == 0) {
				cam_up.x = std::atof(msg->argv[++i]);
				cam_up.y = std::atof(msg->argv[++i]);
				cam_up.z = std::atof(msg->argv[++i]);
			} else if (std::strcmp("--spp", msg->argv[i]) == 0) {
				spp = std::atoi(msg->argv[++i]);
			}
		}
	}

	IMAGE_W = TILE_W * tiles_x;
	IMAGE_H = TILE_H * tiles_y;
	image.resize(IMAGE_W * IMAGE_H * 3, 0);
	num_tiles = tiles_x * tiles_y;

	scene = new SceneMessage(cam_pos, cam_target, cam_up);
	CkPrintf("Rendering %dx%d image with %dx%d tile size\n", IMAGE_W, IMAGE_H, TILE_W, TILE_H);

	if (false) {
		CkPrintf("Rendering image-parallel\n");
		img_tiles = CProxy_ImageParallelTile::ckNew(tiles_x, tiles_y);
		start_pass = start_render = std::chrono::high_resolution_clock::now();
		img_tiles.render();
	} else {
		NUM_REGIONS = 3;
		CkPrintf("Experimental data parallel testing code with %lu regions\n", NUM_REGIONS);
		regions = CProxy_Region::ckNew(NUM_REGIONS);
		region_tiles.resize(num_tiles);
		start_pass = start_render = std::chrono::high_resolution_clock::now();
		regions.load();
	}
}
Main::Main(CkMigrateMessage *msg) {}
void Main::tile_done(const uint64_t x, const uint64_t y, const float *tile) {
	// Write this tiles data into the image
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			for (uint64_t c = 0; c < 3; ++c) {
				image[((i + y) * IMAGE_W + j + x) * 3 + c] += tile[(i * TILE_W + j) * 3 + c];
			}
		}
	}
	++done_count;
	if (done_count == num_tiles) {
		++samples_taken;
		done_count = 0;

		using namespace std::chrono;
		auto end = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(end - start_pass);
		CkPrintf("Iteration took %dms\n", duration.count());
		if (samples_taken == spp) {
			duration = duration_cast<milliseconds>(end - start_render);
			CkPrintf("Rendering took %dms\n", duration.count());

			std::vector<uint8_t> image_out(IMAGE_W * IMAGE_H * 3, 0);
			for (uint64_t i = 0; i < IMAGE_H; ++i) {
				for (uint64_t j = 0; j < IMAGE_W; ++j) {
					for (uint32_t c = 0; c < 3; ++c) {
						const float x = linear_to_srgb(image[(i * IMAGE_W + j) * 3 + c] / spp) * 255.0;
						image_out[(i * IMAGE_W + j) * 3 + c] = glm::clamp(x, 0.f, 255.f);
					}
				}
			}
			stbi_write_png("pathtracer_render.png", IMAGE_W, IMAGE_H, 3, image_out.data(), IMAGE_W * 3);
			CkExit();
		} else {
			start_pass = high_resolution_clock::now();
			img_tiles.render();
		}
	}
}
void Main::tile_done(TileCompleteMessage *msg) {
	const uint64_t tiles_x = IMAGE_W / TILE_W;
	const uint64_t tile_id = msg->tile_y * tiles_x + msg->tile_x;
	auto &tile = region_tiles[tile_id];
	tile.region_tiles.push_back(msg->tile);
	if (msg->tile_owner()) {
		tile.tile_x = msg->tile_x;
		tile.tile_y = msg->tile_y;
		tile.num_tiles = msg->num_other_tiles;
	}
	delete msg;

	if (tile.num_tiles > -1) {
		if (tile.region_tiles.size() == tile.num_tiles) {
			++done_count;
			if (done_count == num_tiles) {
				CkPrintf("Done with distributed rendering of image\n");
				composite_image();
			}
		}
	}
}
void Main::region_loaded() {
	++done_count;
	if (done_count == NUM_REGIONS) {
		CkPrintf("All regions loaded\n");
		done_count = 0;
		regions.render();
	}
}
void Main::composite_image() {
	using namespace std::chrono;
	auto end = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(end - start_pass);
	CkPrintf("Iteration took %dms\n", duration.count());
	duration = duration_cast<milliseconds>(end - start_render);
	CkPrintf("Rendering took %dms\n", duration.count());

	// Go through each tile and composite the regions results.
	// Note that we don't need to do alpha blending here, since we're
	// sending rays if a previous node is transparent it would have continue
	// the ray on to the next node behind it, so we can safely pick the first
	// hit color for each pixel.
	for (const auto &t : region_tiles) {
		// No regions rendered for this tile
		if (t.region_tiles.empty()) {
			continue;
		}

		const uint64_t tile_px_y = t.tile_y * TILE_H;
		const uint64_t tile_px_x = t.tile_x * TILE_W;
		for (uint64_t i = 0; i < TILE_H; ++i) {
			for (uint64_t j = 0; j < TILE_W; ++j) {
				const size_t tx = (i * TILE_W + j) * 4;

				float near_z = std::numeric_limits<float>::infinity();
				size_t first_hit = 0;
				for (size_t r = 0; r < t.region_tiles.size(); ++r) {
					if (t.region_tiles[r][tx + 3] < near_z) {
						near_z = t.region_tiles[r][tx + 3];
						first_hit = r;
					}
				}

				const size_t ix = ((i + tile_px_y) * IMAGE_W + j + tile_px_x) * 3;
				for (size_t c = 0; c < 3; ++c) {
					image[ix + c] = t.region_tiles[first_hit][tx + c];
				}
			}
		}
	}

	// Now convert to sRGB and save the image out
	std::vector<uint8_t> image_out(IMAGE_W * IMAGE_H * 3, 0);
	for (uint64_t i = 0; i < IMAGE_H; ++i) {
		for (uint64_t j = 0; j < IMAGE_W; ++j) {
			const size_t px = (i * IMAGE_W + j) * 3;
			for (uint32_t c = 0; c < 3; ++c) {
				const float x = linear_to_srgb(image[px + c] / spp) * 255.0;
				image_out[px + c] = glm::clamp(x, 0.f, 255.f);
			}
		}
	}
	stbi_write_png("pathtracer_render.png", IMAGE_W, IMAGE_H, 3, image_out.data(), IMAGE_W * 3);
	CkPrintf("Image saved to 'pathtracer_render.png'\n");
	CkExit();
}

SceneMessage::SceneMessage(const glm::vec3 &cam_pos, const glm::vec3 &cam_target, const glm::vec3 &cam_up)
	: cam_pos(cam_pos), cam_target(cam_target), cam_up(cam_up)
{}
void SceneMessage::msg_pup(PUP::er &p) {
	p | cam_pos;
	p | cam_target;
	p | cam_up;
}

#include "main.def.h"

