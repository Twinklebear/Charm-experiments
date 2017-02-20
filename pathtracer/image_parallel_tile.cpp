#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <pup_stl.h>
#include "pt/pt.h"
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

glm::vec3 linear_to_srgb(const glm::vec3 &v) {
	const float a = 0.055;
	const float b = 1.f / 2.4f;
	glm::vec3 srgb(0);
	for (size_t i = 0; i < 3; ++i) {
		if (v[i] <= 0.0031308) {
			srgb[i] = 12.92 * v[i];
		} else {
			srgb[i] = (1.0 + a) * std::pow(v[i], b) - a;
		}
	}
	return srgb;
}

ImageParallelTile::ImageParallelTile() {}
ImageParallelTile::ImageParallelTile(CkMigrateMessage *msg) {}
void ImageParallelTile::render() {
	const uint64_t tiles_x = IMAGE_W / TILE_W;
	const uint64_t start_x = thisIndex.x * TILE_W;
	const uint64_t start_y = thisIndex.y * TILE_H;
	const pt::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);

	// Each tile is RGB8 color data
	// TODO: Each tile should be RGBA8 + ZF32 for compositing primary rays.
	// Maybe for transparency we'd want floating point alpha and color?
	uint8_t *tile = new uint8_t[TILE_W * TILE_H * 3];
	const pt::WhittedIntegrator integrator(pt::Scene({
		std::make_shared<pt::Sphere>(glm::vec3(0), 1.0),
		std::make_shared<pt::Sphere>(glm::vec3(0.25, 0.7, 1.0), 0.25),
		std::make_shared<pt::Plane>(glm::vec3(0, -1, 0), glm::vec3(0, 1, 0))
	}));
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			const float px = j + start_x;
			const float py = i + start_y;
			pt::Ray ray = camera.generate_ray(px, py, {0.5, 0.5});
			const glm::vec3 color = linear_to_srgb(integrator.integrate(ray));
			for (size_t c = 0; c < 3; ++c) {
				tile[(i * TILE_W + j) * 3 + c] = glm::clamp(color[c] * 255.f, 0.f, 255.f);
			}
		}
	}
	main_proxy.tile_done(start_x, start_y, tile);
	delete[] tile;
}

#include "image_parallel_tile.def.h"

