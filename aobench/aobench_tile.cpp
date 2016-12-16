#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <pup_stl.h>
#include "aobench_tile.decl.h"
#include "main.decl.h"
#include "aobench_tile.h"

// readonly global Charm++ variables
extern CProxy_Main main_proxy;
// Image dimensions
extern uint64_t IMAGE_W;
extern uint64_t IMAGE_H;
// Tile x/y dimensions
extern uint64_t TILE_W;
extern uint64_t TILE_H;

AOBenchTile::AOBenchTile(uint64_t ao_samples) : ao_samples(ao_samples), rng(std::random_device{}()),
	spheres({Sphere{vec3f{-2.f, 0.f, -3.5f}, 0.5f}, Sphere{vec3f{-0.5f, 0.f, -3.f}, 0.5f},
			Sphere{vec3f{1.f, 0.f, -2.2f}, 0.5f}}),
	plane(vec3f{0.f, -0.5f, 0.f}, vec3f{0, 1, 0})
{}
AOBenchTile::AOBenchTile(CkMigrateMessage *msg) : ao_samples(8), rng(std::random_device{}()),
	spheres({Sphere{vec3f{-2.f, 0.f, -3.5f}, 0.5f}, Sphere{vec3f{-0.5f, 0.f, -3.f}, 0.5f},
			Sphere{vec3f{1.f, 0.f, -2.2f}, 0.5f}}),
	plane(vec3f{0.f, -0.5f, 0.f}, vec3f{0, 1, 0})
{}
void AOBenchTile::pup(PUP::er &p) {
	p | ao_samples;
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
void AOBenchTile::render_sample() {
	const uint64_t tiles_x = IMAGE_W / TILE_W;
	const uint64_t start_x = thisIndex.x * TILE_W;
	const uint64_t start_y = thisIndex.y * TILE_H;

	float *tile = new float[TILE_W * TILE_H];
	std::uniform_real_distribution<float> distrib;
	for (uint64_t i = 0; i < TILE_H; ++i) {
		for (uint64_t j = 0; j < TILE_W; ++j) {
			const float px = (j + start_x + distrib(rng) - IMAGE_W / 2.f) / (IMAGE_W / 2.f);
			const float py = -(i + start_y + distrib(rng) - IMAGE_H / 2.f) / (IMAGE_H / 2.f);
			// Poor man's perspective projection
			Ray ray;
			ray.origin = 0.f;
			ray.dir.x = px;
			ray.dir.y = py;
			ray.dir.z = -1.f;
			ray.dir = normalized(ray.dir);

			Isect isect;
			isect.t = 1e17f;
			isect.hit = 0;

			for (const auto &s : spheres) {
				s.intersect(isect, ray);
			}
			plane.intersect(isect, ray);
			if (isect.hit){
				// It's just a grayscale image
				tile[i * TILE_W + j] = ambient_occlusion(isect, distrib);
			} else {
				tile[i * TILE_W + j] = 0.f;
			}
		}
	}
	main_proxy.tile_done(start_x, start_y, tile);
	delete[] tile;
}
float AOBenchTile::ambient_occlusion(const Isect &isect, std::uniform_real_distribution<float> &distrib) {
	const vec3f p = isect.p + 0.0001f * isect.n;
	std::array<vec3f, 3> basis;
	ortho_basis(basis, isect.n);
	float occlusion = 0.f;
	for (int j = 0; j < ao_samples; ++j){
		for (int i = 0; i < ao_samples; ++i){
			const float theta = std::sqrt(distrib(rng));
			const float phi = 2.0f * M_PI * distrib(rng);

			const float x = std::cos(phi) * theta;
			const float y = std::sin(phi) * theta;
			const float z = std::sqrt(1.f - theta * theta);

			// Transform from object space to world space
			Ray ray;
			ray.origin = p;
			ray.dir.x = x * basis[0].x + y * basis[1].x + z * basis[2].x;
			ray.dir.y = x * basis[0].y + y * basis[1].y + z * basis[2].y;
			ray.dir.z = x * basis[0].z + y * basis[1].z + z * basis[2].z;

			Isect occluded;
			occluded.t = 1e17f;
			occluded.hit = 0;

			for (const auto &s : spheres) {
				s.intersect(occluded, ray);
			}
			plane.intersect(occluded, ray);

			if (occluded.hit){
				occlusion += 1.f;
			}
		}
	}
	return (ao_samples * ao_samples - occlusion) / (ao_samples * ao_samples);
}

#include "aobench_tile.def.h"

