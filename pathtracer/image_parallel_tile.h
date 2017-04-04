#pragma once

#include <random>

/* This is a pretty bad image-parallel renderer since each processor (core, not node!)
 * has its own copy of the data.
 * TODO: Maybe use node-group or something to load it once per node?
 */
class ImageParallelTile : public CBase_ImageParallelTile {
	std::mt19937 rng;

public:
	ImageParallelTile();
	ImageParallelTile(CkMigrateMessage *msg);

	void pup(PUP::er &p) override;
	// Render a single sample for each pixel in this tile
	void render();
};

