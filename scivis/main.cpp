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
#include "sv/scivis.h"
#include "main.decl.h"
#include "image_parallel_tile.decl.h"
#include "volume_brick.decl.h"
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
	glm::uvec3 dims, bricking(0);
	sv::VolumeDType dtype;
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
			} else if (std::strcmp("--vol", msg->argv[i]) == 0) {
				volume = msg->argv[++i];
				dims.x = std::atoi(msg->argv[++i]);
				dims.y = std::atoi(msg->argv[++i]);
				dims.z = std::atoi(msg->argv[++i]);
				dtype = sv::parse_volume_dtype(msg->argv[++i]);
			} else if (std::strcmp("--brick", msg->argv[i]) == 0) {
				bricking.x = std::atoi(msg->argv[++i]);
				bricking.y = std::atoi(msg->argv[++i]);
				bricking.z = std::atoi(msg->argv[++i]);
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
			}
		}
	}
	if (volume.empty()) {
		CkPrintf("Error: A volume file specified with --vol is required.\n%s\n", USAGE.c_str());
	}
	if (cam_pos == glm::vec3(0)) {
		cam_pos = glm::vec3(dims.x / 2.f, dims.y / 2.f, dims.z * 2.f);
	}
	if (cam_target == glm::vec3(0)) {
		cam_target = glm::vec3(dims.x / 2.f, dims.y / 2.f, dims.z / 2.f);
	}

	IMAGE_W = TILE_W * tiles_x;
	IMAGE_H = TILE_H * tiles_y;
	image.resize(IMAGE_W * IMAGE_H * 3, 0);
	num_tiles = tiles_x * tiles_y;

	scene = new SceneMessage(volume, dims, bricking, dtype, cam_pos, cam_target, cam_up);
	CkPrintf("Rendering %dx%d image with %dx%d tile size\n", IMAGE_W, IMAGE_H, TILE_W, TILE_H);

	if (!scene->data_parallel()) {
		CkPrintf("Rendering image-parallel\n");
		CProxy_ImageParallelTile img_tiles = CProxy_ImageParallelTile::ckNew(tiles_x, tiles_y);
		img_tiles.render();
	} else {
		brick_tiles.resize(tiles_x * tiles_y);
		CkPrintf("Rendering data-parallel with %dx%dx%d bricking\n", bricking.x, bricking.y, bricking.z);
		CProxy_VolumeBrick bricks = CProxy_VolumeBrick::ckNew(bricking.x, bricking.y, bricking.z);
		bricks.render();
	}
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
void Main::brick_tile_done(const uint64_t tile_x, const uint64_t tile_y, const float *tile) {
	const size_t tid = tile_y * (IMAGE_W / TILE_W) + tile_x;
	brick_tiles[tid].emplace_back(tile, tile + TILE_W * TILE_H * 5);
	// If we've finished this tile, composite it and write it to the final framebuffer
	const uint64_t start_x = tile_x * TILE_W;
	const uint64_t start_y = tile_y * TILE_H;
	if (brick_tiles[tid].size() == scene->num_bricks()) {
		using Fragment = std::pair<glm::vec4, float>;
		for (uint64_t i = 0; i < TILE_H; ++i) {
			for (uint64_t j = 0; j < TILE_W; ++j) {
				// Collect and sort the fragments for this pixel
				// Note: I don't think sorting by depth per-pixel is actually needed for pure
				// distributed volume rendering b/c the sort can be done on the brick level. However
				// I think for mixed volume and geometry data you do need this?
				// It might not actually be necessary even for mixed volume and geometry data?
				std::vector<Fragment> fragments;
				for (const auto &t : brick_tiles[tid]) {
					const float *vals = &t[(i * TILE_W + j) * 5];
					fragments.push_back(std::make_pair(glm::vec4(vals[0], vals[1], vals[2], vals[3]), vals[4]));
				}
				std::sort(fragments.begin(), fragments.end(),
					[](const Fragment &a, const Fragment &b) {
						return a.second < b.second;
					});
				// Blend the fragments and write to the final image
				glm::vec4 color = glm::vec4(0);
				for (const auto &f : fragments) {
					glm::vec3 col = (1.f - color.w) * f.first.w * glm::vec3(f.first);
					color.x += col.x;
					color.y += col.y;
					color.z += col.z;
					color.w += (1.0 - color.w) * f.first.w;
					if (color.w >= 0.98) {
						break;
					}
				}
				// Composite onto the background
				glm::vec3 final_col = glm::vec3(color) + (1.f - color.w) * glm::vec3(1);
				for (int c = 0; c < 3; ++c) {
					image[((i + start_y) * IMAGE_W + j + start_x) * 3 + c] = final_col[c] * 255.f;
				}
			}
		}
		// Release the tiles since we no longer need them
		brick_tiles[tid].clear();
		++done_count;
		if (done_count == num_tiles) {
			stbi_write_png("scivis_render.png", IMAGE_W, IMAGE_H, 3, image.data(), IMAGE_W * 3);
			CkExit();
		}
	}
}

SceneMessage::SceneMessage() {}
SceneMessage::SceneMessage(const std::string &vol_file, const glm::uvec3 &dims, const glm::uvec3 &bricking,
		sv::VolumeDType dtype, const glm::vec3 &cam_pos, const glm::vec3 &cam_target,
		const glm::vec3 &cam_up)
	: vol_file(vol_file), dims(dims), bricking(bricking), dtype(dtype),
	cam_pos(cam_pos), cam_target(cam_target), cam_up(cam_up)
{}
void SceneMessage::msg_pup(PUP::er &p) {
	p | vol_file;
	p | dims;
	p | bricking;
	p | dtype;
	p | cam_pos;
	p | cam_target;
	p | cam_up;
	if (p.isUnpacking() && !data_parallel()) {
		CkPrintf("SceneMessage unpacking and allocating volume\n");
		volume = sv::load_raw_volume(scene->vol_file, scene->dims, scene->dtype);
	}
}
bool SceneMessage::data_parallel() const {
	return bricking != glm::uvec3(0);
}
uint64_t SceneMessage::num_bricks() const {
	return bricking.x * bricking.y * bricking.z;
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

