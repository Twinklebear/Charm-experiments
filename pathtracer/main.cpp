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

	CkPrintf("Rendering image-parallel\n");
	img_tiles = CProxy_ImageParallelTile::ckNew(tiles_x, tiles_y);
	start_pass = start_render = std::chrono::high_resolution_clock::now();
	img_tiles.render();
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

SceneMessage::SceneMessage() {}
SceneMessage::SceneMessage(const glm::vec3 &cam_pos, const glm::vec3 &cam_target, const glm::vec3 &cam_up)
	: cam_pos(cam_pos), cam_target(cam_target), cam_up(cam_up)
{}
void SceneMessage::msg_pup(PUP::er &p) {
	p | cam_pos;
	p | cam_target;
	p | cam_up;
}
void* SceneMessage::pack(SceneMessage *msg) {
	PUP::sizer sizer;
	msg->msg_pup(sizer);

	void *buf = CkAllocBuffer(msg, sizer.size());

	PUP::toMem to_mem(buf);
	msg->msg_pup(to_mem);
	delete msg;
	return buf;
}
SceneMessage* SceneMessage::unpack(void *buf) {
	SceneMessage *msg = static_cast<SceneMessage*>(CkAllocBuffer(buf, sizeof(SceneMessage)));
	msg = new ((void*)msg) SceneMessage();
	PUP::fromMem from_mem(buf);
	msg->msg_pup(from_mem);
	CkFreeMsg(buf);
	return msg;
}

#include "main.def.h"

