#pragma once

#include <array>
#include "scene.h"

class AOBenchTile : public CBase_AOBenchTile {
	uint64_t ao_samples;
	std::array<Sphere, 3> spheres;
	Plane plane;

public:
	AOBenchTile(uint64_t ao_samples);
	AOBenchTile(CkMigrateMessage *msg);

	//virtual void pup(PUP::er &p);
	// Entry method for rendering a sample for this chare's tile.
	void render_sample();
};

