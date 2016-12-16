#pragma once

#include <random>
#include <array>
#include "scene.h"

class AOBenchTile : public CBase_AOBenchTile {
	uint64_t ao_samples;
	std::mt19937 rng;
	std::array<Sphere, 3> spheres;
	Plane plane;

public:
	AOBenchTile(uint64_t ao_samples);
	AOBenchTile(CkMigrateMessage *msg);

	void pup(PUP::er &p) override;
	// Entry method for rendering a sample for this chare's tile.
	void render_sample();

private:
	// Compute the ambient occlusion value for a hit point
	float ambient_occlusion(const Isect &isect, std::uniform_real_distribution<float> &distrib);
};

