#include <memory>
#include <random>
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

ImageParallelTile::ImageParallelTile() : rng(std::random_device()()) {}
ImageParallelTile::ImageParallelTile(CkMigrateMessage *msg) : rng(std::random_device()()) {
	delete msg;
}
void ImageParallelTile::pup(PUP::er &p) {
	// Migrate the RNG state
	if (p.isUnpacking()) {
		std::string rng_str;
		p | rng_str;
		std::stringstream rng_state(rng_str);
		rng_state >> rng;
	} else {
		std::stringstream rng_state;
		rng_state << rng;
		std::string rng_str = rng_state.str();
		p | rng_str;
	}
}
void ImageParallelTile::render() {
	const uint64_t tiles_x = IMAGE_W / TILE_W;
	const uint64_t start_x = thisIndex.x * TILE_W;
	const uint64_t start_y = thisIndex.y * TILE_H;
	const pt::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);

	std::shared_ptr<pt::BxDF> lambertian_blue = std::make_shared<pt::Lambertian>(glm::vec3(0.1, 0.1, 0.8));
	std::shared_ptr<pt::BxDF> lambertian_white = std::make_shared<pt::Lambertian>(glm::vec3(0.8));
	std::shared_ptr<pt::BxDF> lambertian_red = std::make_shared<pt::Lambertian>(glm::vec3(0.8, 0.1, 0.1));
	std::shared_ptr<pt::BxDF> reflective = std::make_shared<pt::SpecularReflection>(glm::vec3(0.8));

	// TODO: Each tile should be also have Z for compositing primary rays.
	float *tile = new float[TILE_W * TILE_H * 3];
	const pt::PathIntegrator integrator(glm::vec3(0.05), pt::Scene({
			std::make_shared<pt::Sphere>(glm::vec3(0), 1.0, lambertian_blue),
			std::make_shared<pt::Sphere>(glm::vec3(1.0, 0.7, 1.0), 0.25, lambertian_blue),
			std::make_shared<pt::Sphere>(glm::vec3(-1, -0.75, 1.2), 0.5, lambertian_red),
			// Walls
			std::make_shared<pt::Plane>(glm::vec3(0, -1, 0), glm::vec3(0, 1, 0), 4, lambertian_white),
			std::make_shared<pt::Plane>(glm::vec3(0, 2, 0), glm::vec3(0, -1, 0), 4, lambertian_white),
			std::make_shared<pt::Plane>(glm::vec3(-1.5, 0, 0), glm::vec3(1, 0, 0), 4, lambertian_white),
			std::make_shared<pt::Plane>(glm::vec3(1.5, 0, 0), glm::vec3(-1, 0, 0), 4, lambertian_white),
			std::make_shared<pt::Plane>(glm::vec3(0, 0, -2), glm::vec3(0, 0, 1), 4, lambertian_white)
		},
		{
			std::make_shared<pt::PointLight>(glm::vec3(0, 1.5, 0.5), glm::vec3(0.9)),
		}
	));

	std::uniform_real_distribution<float> real_distrib;
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			const float px = j + start_x;
			const float py = i + start_y;
			pt::Ray ray = camera.generate_ray(px, py, {real_distrib(rng), real_distrib(rng)});
			const glm::vec3 color = integrator.integrate(ray);
			for (size_t c = 0; c < 3; ++c) {
				tile[(i * TILE_W + j) * 3 + c] = color[c];
			}
		}
	}
	main_proxy.tile_done(start_x, start_y, tile);
	delete[] tile;
}

#include "image_parallel_tile.def.h"

