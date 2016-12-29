#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <pup_stl.h>
#include "sv/scivis.h"
#include "pup_operators.h"
#include "main.decl.h"
#include "main.h"
#include "image_parallel_tile.decl.h"
#include "image_parallel_tile.h"

// readonly global Charm++ variables
extern CProxy_Main main_proxy;
extern SceneMessage *scene;
extern uint64_t IMAGE_W;
extern uint64_t IMAGE_H;
extern uint64_t TILE_W;
extern uint64_t TILE_H;

ImageParallelTile::ImageParallelTile() {}
ImageParallelTile::ImageParallelTile(CkMigrateMessage *msg) {}
void ImageParallelTile::render() {
	const uint64_t tiles_x = IMAGE_W / TILE_W;
	const uint64_t start_x = thisIndex.x * TILE_W;
	const uint64_t start_y = thisIndex.y * TILE_H;
	// TODO: Need a scene data message to send, or struct for the ctor
	const sv::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);

	const sv::RaycastRender renderer(0.5, scene->volume);
	// Each tile is RGB8 color data
	uint8_t *tile = new uint8_t[TILE_W * TILE_H * 3];
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			const float px = j + start_x;
			const float py = i + start_y;
			sv::Ray ray = camera.generate_ray(px, py, {0.5, 0.5});
			const glm::vec4 vol_color = renderer.render(ray);
			glm::vec3 col = glm::vec3(vol_color) + (1.f - vol_color.w) * glm::vec3(1);
			// Composite onto the background color
			for (size_t c = 0; c < 3; ++c) {
				tile[(i * TILE_W + j) * 3 + c] = col[c] * 255.f;
			}
		}
	}
	main_proxy.tile_done(start_x, start_y, tile);
}

#include "image_parallel_tile.def.h"

